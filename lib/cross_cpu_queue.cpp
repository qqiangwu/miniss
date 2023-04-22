#include "miniss/cross_cpu_queue.h"
#include "miniss/cpu.h"

using namespace miniss;

bool Cross_cpu_queue::poll_tx()
{
    assert(this_cpu() == &to_);

    return tx_.process([this](Work_item* item) { item->process().then([this, item] { respond_(item); }); }) > 0;
}

bool Cross_cpu_queue::poll_rx()
{
    assert(this_cpu() == &from_);

    return rx_.process([](Work_item* item) {
        std::unique_ptr<Work_item> p(item);
        item->complete();
    }) > 0;
}

bool Cross_cpu_queue::flush_tx()
{
    assert(this_cpu() == &from_);

    if (tx_.flush()) {
        to_.maybe_wakeup();
        return true;
    }

    return false;
}

bool Cross_cpu_queue::flush_rx()
{
    assert(this_cpu() == &to_);

    if (rx_.flush()) {
        from_.maybe_wakeup();
        return true;
    }

    return false;
}

void Cross_cpu_queue::submit_(Work_item* item)
{
    assert(this_cpu() == &from_);

    if (tx_.push(item)) {
        to_.maybe_wakeup();
    }
}

void Cross_cpu_queue::respond_(Work_item* item)
{
    assert(this_cpu() == &to_);

    if (rx_.push(item)) {
        from_.maybe_wakeup();
    }
}