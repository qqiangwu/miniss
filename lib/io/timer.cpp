#include <algorithm>
#include <cassert>
#include <spdlog/spdlog.h>
#include "miniss/cpu.h"
#include "miniss/util.h"
#include "miniss/io/timer.h"

using namespace miniss;

namespace {

timespec to_timespec(Clock_type::time_point t)
{
    using ns = std::chrono::nanoseconds;
    auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
    return { n / 1'000'000'000, n % 1'000'000'000 };
}

}

Timer_service::Timer_service(CPU& cpu)
    : cpu_(cpu)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    int r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
    throw_system_error_if(r < 0, "block SIGALRM failed");

    // @fixme error handling here

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD_ID;
    sev._sigev_un._tid = syscall(SYS_gettid);
    sev.sigev_signo = SIGALRM;
    r = ::timer_create(CLOCK_MONOTONIC, &sev, &physical_timer_);
    throw_system_error_if(r < 0, "create timer failed");
}

Timer_service::~Timer_service()
{
    timer_delete(physical_timer_);
    physical_timer_ = {};
}

void Timer_service::add_timer(Clock_type::duration interval, std::unique_ptr<task> t)
{
    const auto new_deadline = Clock_type::now() + interval;
    const auto old_deadline = timers_.empty()? Clock_type::time_point::max(): timers_.begin()->first;
    timers_[new_deadline].push_back(std::move(t));

    if (new_deadline < old_deadline) {
        rearm_timer_();
    }
}

void Timer_service::complete_timers()
{
    const auto now = Clock_type::now();

    const auto ub = timers_.upper_bound(now);
    assert(ub == timers_.end() || ub->first > now);

    // @fixme error handle here
    std::for_each(timers_.begin(), ub, [cpu = &cpu_](auto& kv){
        for (auto& task: kv.second) {
            cpu->schedule(std::move(task));
        }
    });

    timers_.erase(timers_.begin(), ub);

    if (!timers_.empty()) {
        rearm_timer_();
    }
}

void Timer_service::rearm_timer_()
{
    assert(!timers_.empty());

    itimerspec its;
    its.it_interval = {};
    its.it_value = to_timespec(timers_.begin()->first);
    const auto ret = timer_settime(physical_timer_, TIMER_ABSTIME, &its, NULL);
    if (ret < 0) {
        spdlog::critical("set timer failed: {}", errno);
        std::terminate();
    }
}