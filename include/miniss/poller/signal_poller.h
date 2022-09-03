#pragma once

#include <atomic>
#include <functional>
#include <map>
#include "miniss/poller.h"

namespace miniss {

class Signal_poller : public Poller {
public:
    using Signal_handler = std::function<void()>;

    explicit Signal_poller(std::atomic_uint64_t* pending_signals)
        : pending_signals_(pending_signals)
    {}

    ~Signal_poller() override;

    bool poll() override;
    bool pure_poll() override;

    void register_signal(int signo, Signal_handler&& handler);

private:
    std::atomic_uint64_t* pending_signals_;
    std::map<int, Signal_handler> signal_handlers_;
};

}