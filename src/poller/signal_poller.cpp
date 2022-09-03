#include <signal.h>
#include <fmt/core.h>
#include "miniss/util.h"
#include "miniss/cpu.h"
#include "miniss/poller/signal_poller.h"

using namespace miniss;

Signal_poller::~Signal_poller()
{
    sigset_t mask;
    sigfillset(&mask);
    ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

bool Signal_poller::poll()
{
    const auto signals = pending_signals_->load(std::memory_order_relaxed);
    if (!signals) {
        return false;
    }

    pending_signals_->fetch_and(~signals, std::memory_order_relaxed);
    for (size_t i = 0; i < sizeof(std::uint64_t) * 8; ++i) {
        if (signals & (1ULL << i)) {
            auto it = signal_handlers_.find(i);
            if (it == signal_handlers_.end()) {
                continue;
            }

            it->second();
        }
    }

    return true;
}

bool Signal_poller::pure_poll()
{
    return pending_signals_->load(std::memory_order_relaxed) != 0;
}

void Signal_poller::register_signal(int signo, Signal_handler&& handler)
{
    if (signal_handlers_.contains(signo)) {
        throw std::runtime_error{fmt::format("signal {} already registered", signo)};
    }

    sigset_t set;
    sigemptyset(&set);

    struct sigaction sa;
    sa.sa_sigaction = dispatch_signal;
    sa.sa_mask = set;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    auto r = ::sigaction(signo, &sa, nullptr);
    throw_system_error_if(r != 0);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signo);
    r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    throw_system_error_if(r != 0);

    signal_handlers_.emplace(signo, std::move(handler));
}