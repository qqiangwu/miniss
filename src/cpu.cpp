#include <signal.h>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include "miniss/cpu.h"
#include "miniss/util.h"
#include "miniss/poller/signal_poller.h"
#include "miniss/poller/ipc_poller.h"

using namespace std::chrono_literals;
using namespace miniss;

void miniss::dispatch_signal(int signo, siginfo_t* siginfo, void* ignore)
{
    this_cpu()->pending_signals_.fetch_or(1ull << signo, std::memory_order_relaxed);
}

CPU::CPU(const Configuration&, int cpu_id)
    : cpu_id_(cpu_id), timer_service_(*this)
{
}

int CPU::run()
{
    init_pollers_();

    while (true) {
        for (auto& poller: pollers_) {
            poller->poll();
        }

        if (task_queue_.empty()) {
            std::this_thread::sleep_for(1s);
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

void CPU::schedule_after(Clock_type::duration interval, std::unique_ptr<task> t)
{
    timer_service_.add_timer(interval, std::move(t));
}

void CPU::init_pollers_()
{
    auto signal_poller = std::make_unique<Signal_poller>(&pending_signals_);
    signal_poller->register_signal(SIGALRM, [this]{
        timer_service_.complete_timers();
    });

    pollers_.push_back(std::move(signal_poller));
    pollers_.push_back(std::make_unique<Ipc_poller>(cpu_id()));
}