#pragma once

#include <signal.h>
#include <atomic>
#include <cassert>
#include <functional>
#include <deque>
#include <memory>
#include <vector>
#include <chrono>
#include "miniss/configuration.h"
#include "miniss/poller.h"
#include "miniss/io/timer.h"

namespace miniss {

class CPU {
public:
    CPU(const Configuration& conf, int cpu_id);

    void run();

    void schedule(Task&& task)
    {
        task_queue_.push_back(std::move(task));
    }

    void schedule_after(Clock_type::duration interval, Task&& task);

private:
    void init_pollers_();

private:
    int cpu_id_ = 0;

    std::atomic<std::uint64_t> pending_signals_ = {};
    std::deque<Task> task_queue_;
    std::vector<std::unique_ptr<Poller>> pollers_;

    Timer_service timer_service_;

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