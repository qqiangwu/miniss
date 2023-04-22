#include "miniss/cpu.h"
#include "miniss/poller/file_io_poller.h"
#include "miniss/poller/ipc_poller.h"
#include "miniss/poller/signal_poller.h"
#include "miniss/poller/syscall_poller.h"
#include "miniss/util.h"
#include <array>
#include <chrono>
#include <fcntl.h>
#include <fmt/color.h>
#include <fmt/std.h>
#include <nonstd/scope.hpp>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <thread>

using namespace std::chrono_literals;
using namespace miniss;

void miniss::dispatch_signal(int signo, siginfo_t* siginfo, void* ignore)
{
    this_cpu()->pending_signals_.fetch_or(1ull << signo, std::memory_order_relaxed);
}

CPU::CPU(const Configuration&, int cpu_id)
    : cpu_id_(cpu_id)
    , timer_service_(*this)
    , syscall_runner_(*this)
{
    epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    throw_system_error_if(epoll_fd_ < 0, "epoll create failed");

    // @todo fixme, epoll is trigger once only
    ::epoll_event eevt;
    eevt.events = EPOLLIN;
    eevt.data.ptr = nullptr;
    int r = ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, file_io_.get_eventfd(), &eevt);
    assert(r == 0 && "it won't be wrong unless bug");
}

CPU::~CPU()
{
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

int CPU::run()
{
    init_pollers_();

    while (true) {
        bool cpu_active = false;

        for (auto& poller : pollers_) {
            cpu_active |= poller->poll();
        }

        if (task_queue_.empty() && !cpu_active) {
            run_idle_proc_();
            continue;
        }

        constexpr int batch_size = 8;
        for (int i = 0; i < batch_size && !task_queue_.empty(); ++i) {
            auto fn = std::move(task_queue_.front());
            task_queue_.pop_front();

            try {
                fn->run();
            } catch (...) {
                spdlog::error("run task failed: {}", current_exception_message());
            }
        }
    }

    return 0;
}

void CPU::wakeup() { ::pthread_kill(thread_id_, SIGALRM); }

void CPU::maybe_wakeup()
{
    // @fixme remove unnecessary wakeup
    wakeup();
}

void CPU::schedule_after(Clock_type::duration interval, std::unique_ptr<task> t)
{
    timer_service_.add_timer(interval, std::move(t));
}

future<File> CPU::open_file(const std::filesystem::path& p, int open_options)
{
    return submit_syscall([p, open_options] {
        int fd = ::open(p.c_str(), open_options | O_CLOEXEC | O_DIRECT, 0644);
        throw_system_error_if(fd < 0, fmt::format("open file {} failed", p).c_str());

        return wrap_syscall(fd);
    }).then([this](auto fd) { return File(this, &file_io_, fd); });
}

void CPU::init_pollers_()
{
    auto signal_poller = std::make_unique<Signal_poller>(&pending_signals_);
    signal_poller->register_signal(SIGALRM, [this] { timer_service_.complete_timers(); });

    pollers_.push_back(std::move(signal_poller));
    pollers_.push_back(std::make_unique<Ipc_poller>(cpu_id()));
    pollers_.push_back(std::make_unique<Syscall_poller>(syscall_runner_));
    pollers_.push_back(std::make_unique<File_io_poller>(file_io_));
}

void CPU::run_idle_proc_()
{
    sigset_t mask;
    sigset_t active_mask;
    sigemptyset(&mask);
    sigfillset(&mask);
    ::pthread_sigmask(SIG_SETMASK, &mask, &active_mask);
    nonstd::scope_exit guard([&] { ::pthread_sigmask(SIG_SETMASK, &active_mask, nullptr); });

    if (pending_signals_) {
        return;
    }

    // @fixme atomicty
    std::array<epoll_event, 128> eevt;
    int nr = ::epoll_pwait(epoll_fd_, eevt.data(), eevt.size(), -1, &active_mask);
    if (nr == -1 && errno == EINTR) {
        /* empty now */
    }

    return;
}