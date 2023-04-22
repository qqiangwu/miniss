#pragma once

#include "miniss/future.h"
#include "miniss/util/unbounded_spsc_queue.h"
#include <cassert>
#include <deque>
#include <memory>
#include <mutex>
#include <type_traits>

namespace miniss {

class CPU;

class Cross_cpu_queue {
public:
    Cross_cpu_queue(CPU& from, CPU& to)
        : from_(from)
        , to_(to)
    {
    }

    bool poll_tx();
    bool poll_rx();

    bool flush_tx();
    bool flush_rx();

    template <class Fn> futurize_t<std::result_of_t<Fn()>> submit(Fn&& fn)
    {
        auto task = std::make_unique<Work_item_impl<Fn>>(std::forward<Fn>(fn));
        auto fut = task->promise_.get_future();

        submit_(task.get());

        task.release();
        return fut;
    }

private:
    void submit_(Work_item* item);
    void respond_(Work_item* item);

private:
    CPU& from_;
    CPU& to_;

    Unbounded_spsc_queue tx_;
    Unbounded_spsc_queue rx_;
};

}