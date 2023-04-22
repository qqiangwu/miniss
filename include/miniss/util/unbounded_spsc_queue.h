#pragma once

#include "miniss/future.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <deque>
#include <variant>

namespace miniss {

struct Work_item {
    virtual ~Work_item() = default;
    virtual future<> process() = 0;
    virtual void complete() = 0;
};

template <class Fn> struct Work_item_impl final : public Work_item {
    using futurator = futurize<std::result_of_t<Fn()>>;
    using future_type = typename futurator::type;
    using value_type = typename future_type::value_type;
    using promise_type = typename futurator::promise_type;

    Fn fn_;
    std::variant<std::monostate, value_type, std::exception_ptr> ret_;
    promise_type promise_;

    explicit Work_item_impl(Fn&& fn)
        : fn_(std::move(fn))
    {
    }

    future<> process() override
    {
        return futurator::apply(fn_).then_wrapped([this](auto&& r) {
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

struct Unbounded_spsc_queue {
    std::deque<Work_item*> queue_buffer;
    boost::lockfree::spsc_queue<Work_item*, boost::lockfree::capacity<1024>> queue;

    bool push(Work_item* item);
    bool flush();

    template <class Fn> size_t process(Fn&& fn)
    {
        Work_item* buf[8];

        const auto nr = queue.pop(buf);

        std::for_each_n(std::begin(buf), nr, fn);

        return nr;
    }
};

}