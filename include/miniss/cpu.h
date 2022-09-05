#pragma once

#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <cassert>
#include <functional>
#include <deque>
#include <memory>
#include <vector>
#include <chrono>
#include "miniss/configuration.h"
#include "miniss/poller.h"
#include "miniss/task.h"
#include "miniss/io/timer.h"

namespace miniss {

class CPU {
public:
    CPU(const Configuration& conf, int cpu_id);

    ~CPU();

    int run();

    void wakeup();
    void maybe_wakeup();

    template <class Fn>
    void schedule(Fn&& task)
    {
        schedule(make_task(std::forward<Fn>(task)));
    }

    void schedule(std::unique_ptr<task> t)
    {
        task_queue_.push_back(std::move(t));
    }

    template <class Fn>
    void schedule_after(Clock_type::duration interval, Fn&& fn)
    {
        schedule_after(interval, make_task(std::forward<Fn>(fn)));
    }

    void schedule_after(Clock_type::duration interval, std::unique_ptr<task> t);

    unsigned cpu_id() const { return cpu_id_; }

private:
    void init_pollers_();

    void run_idle_proc_();

private:
    int cpu_id_ = 0;
    int epoll_fd_ = -1;

    std::atomic<std::uint64_t> pending_signals_ = {};
    std::deque<std::unique_ptr<task>> task_queue_;
    std::vector<std::unique_ptr<Poller>> pollers_;

    Timer_service timer_service_;

    alignas(64) const pthread_t thread_id_ = ::pthread_self();

private:
    friend void dispatch_signal(int signo, siginfo_t* siginfo, void* ignore);
};

inline thread_local CPU* local_cpu_;

inline CPU* this_cpu()
{
    assert(local_cpu_);
    return local_cpu_;
}

void dispatch_signal(int signo, siginfo_t* siginfo, void* ignore);

}