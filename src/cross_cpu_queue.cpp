#include "miniss/cross_cpu_queue.h"
#include "miniss/cpu.h"

using namespace miniss;

bool Cross_cpu_queue::poll_tx()
{
    assert(this_cpu() == &to_);

    return tx_.process([this](Work_item* item){
        item->process().then([this, item]{
            respond_(item);
        });
    }) > 0;
}

bool Cross_cpu_queue::poll_rx()
{
    assert(this_cpu() == &from_);

    return rx_.process([](Work_item* item){
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

bool Cross_cpu_queue::Unbounded_spsc_queue::push(Work_item* item)
{
    queue_buffer.push_back(item);
    if (queue_buffer.size() < 8) {
        return false;
    }

    return flush();
}

bool Cross_cpu_queue::Unbounded_spsc_queue::flush()
{
    int n = 0;
    while (queue.write_available() && !queue_buffer.empty()) {
        queue.push(queue_buffer.front());
        queue_buffer.pop_front();
        ++n;
    }

    return n > 0;
}

template <class Fn>
size_t Cross_cpu_queue::Unbounded_spsc_queue::process(Fn&& fn)
{
    Work_item* buf[8];

    const auto nr = queue.pop(buf);

    std::for_each_n(std::begin(buf), nr, fn);

    return nr;
}