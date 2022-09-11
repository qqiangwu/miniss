#pragma once

#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>
#include <chrono>
#include <filesystem>
#include <boost/noncopyable.hpp>
#include "miniss/configuration.h"
#include "miniss/poller.h"
#include "miniss/task.h"
#include "miniss/io/syscall.h"
#include "miniss/io/timer.h"
#include "miniss/io/file.h"
#include "miniss/io/file_io.h"

namespace miniss {

class CPU : private boost::noncopyable {
public:
    CPU(const Configuration& conf, int cpu_id);

    ~CPU();

public:
    unsigned cpu_id() const { return cpu_id_; }

public:
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

public:
    template <class Fn>
    auto submit_syscall(Fn&& fn)
    {
        return syscall_runner_.submit(std::forward<Fn>(fn));
    }

public:
    // file operations
    future<File> open_file(const std::filesystem::path& p, int open_options);

private:
    void init_pollers_();

    void run_idle_proc_();

private:
    alignas(64) const pthread_t thread_id_ = ::pthread_self();

    int cpu_id_ = 0;
    int epoll_fd_ = -1;

    std::atomic<std::uint64_t> pending_signals_ = {};
    std::deque<std::unique_ptr<task>> task_queue_;
    std::vector<std::unique_ptr<Poller>> pollers_;

    Timer_service timer_service_;
    Syscall_runner syscall_runner_;
    File_io file_io_;

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