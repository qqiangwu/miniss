#include "miniss/util/unbounded_spsc_queue.h"

using namespace miniss;

bool Unbounded_spsc_queue::push(Work_item* item)
{
    queue_buffer.push_back(item);
    if (queue_buffer.size() < 8) {
        return false;
    }

    return flush();
}

bool Unbounded_spsc_queue::flush()
{
    int n = 0;
    while (queue.write_available() && !queue_buffer.empty()) {
        queue.push(queue_buffer.front());
        queue_buffer.pop_front();
        ++n;
    }

    return n > 0;
}