#pragma once

#include <cassert>
#include <deque>
#include <mutex>
#include <memory>
#include <type_traits>
#include <variant>
#include <boost/lockfree/spsc_queue.hpp>
#include "miniss/future.h"

namespace miniss {

class CPU;

class Cross_cpu_queue {
public:
    Cross_cpu_queue(CPU& from, CPU& to)
        : from_(from), to_(to)
    {
    }

    bool poll_tx();
    bool poll_rx();

    bool flush_tx();
    bool flush_rx();

    template <class Fn>
    futurize_t<std::result_of_t<Fn()>> submit(Fn&& fn)
    {
        auto task = std::make_unique<Work_item_impl<Fn>>(std::forward<Fn>(fn));
        auto fut = task->promise_.get_future();

        submit_(task.get());

        task.release();
        return fut;
    }

private:
    struct Work_item
    {
        virtual ~Work_item() = default;
        virtual future<> process() = 0;
        virtual void complete() = 0;
    };

    template <class Fn>
    struct Work_item_impl : public Work_item
    {
        using futurator = futurize<std::result_of_t<Fn()>>;
        using future_type = typename futurator::type;
        using value_type = typename future_type::value_type;
        using promise_type = typename futurator::promise_type;

        Fn fn_;
        std::variant<std::monostate, value_type, std::exception_ptr> ret_;
        promise_type promise_;

        explicit Work_item_impl(Fn&& fn): fn_(std::move(fn))
        {}

        future<> process() override
        {
            return futurator::apply(fn_).then_wrapped([this](auto&& r){
                try {
                    ret_.template emplace<value_type>(std::move(r.get()));
                } catch (...) {
                    ret_.template emplace<std::exception_ptr>(std::current_exception());
                }
            });
        }

        void complete() override
        {
            switch (ret_.index()) {
            case 1:
                promise_.set_value(std::move(std::get<value_type>(ret_)));
                break;

            case 2:
                promise_.set_exception(std::move(std::get<std::exception_ptr>(ret_)));
                break;

            default:
                assert(!"unreachable");
                std::abort();
            }
        }
    };

    void submit_(Work_item* item);
    void respond_(Work_item* item);

private:
    CPU& from_;
    CPU& to_;

    struct Unbounded_spsc_queue {
        std::deque<Work_item*> queue_buffer;
        boost::lockfree::spsc_queue<Work_item*,  boost::lockfree::capacity<1024>> queue;

        bool push(Work_item* item);
        bool flush();

        template <class Fn>
        size_t process(Fn&& fn);
    };

    Unbounded_spsc_queue tx_;
    Unbounded_spsc_queue rx_;
};

}