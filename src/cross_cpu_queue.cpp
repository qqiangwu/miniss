#include "miniss/cross_cpu_queue.h"

using namespace miniss;

bool Cross_cpu_queue::poll_tx()
{
    auto item = [this]()-> Work_item* {
        std::lock_guard _(tx_mut_);
        if (tx_.empty()) {
            return nullptr;
        }

        auto r = tx_.front();
        tx_.pop_front();

        return r;
    }();

    if (!item) {
        return false;
    }

    item->process().then([this, item]{
        respond_(item);
    });

    return true;
}

bool Cross_cpu_queue::poll_rx()
{
    auto item = [this]() -> Work_item* {
        std::lock_guard _(rx_mut_);
        if (rx_.empty()) {
            return nullptr;
        }

        auto r = rx_.front();
        rx_.pop_front();

        return r;
    }();

    if (!item) {
        return false;
    }

    std::unique_ptr<Work_item> p(item);
    item->complete();

    return true;
}

void Cross_cpu_queue::submit_(Work_item* item)
{
    std::lock_guard _(tx_mut_);
    tx_.push_back(item);
}

void Cross_cpu_queue::respond_(Work_item* item)
{
    std::lock_guard _(rx_mut_);
    rx_.push_back(item);
}