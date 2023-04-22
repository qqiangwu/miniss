#pragma once

#include "miniss/future.h"
#include <memory>
#include <vector>

namespace miniss {

namespace detail {

    template <class Fut> struct When_all_state : public std::enable_shared_from_this<When_all_state<Fut>> {
        using Future_t = future<std::vector<Fut>>;
        using Promise_t = typename Future_t::promise_type;

        std::vector<Fut> futures_;
        Promise_t promise_;

        explicit When_all_state(std::vector<Fut>&& futs)
            : futures_(std::move(futs))
        {
        }

        ~When_all_state() { promise_.set_value(std::move(futures_)); }

        Future_t get_future() { return promise_.get_future(); }

        Future_t wait()
        {
            for (auto& f : futures_) {
                f = f.then_wrapped([s = this->shared_from_this()](auto&& fut) { return std::move(fut); });
            }

            return promise_.get_future();
        }
    };

}

template <class Fut> future<std::vector<Fut>> when_all(std::vector<Fut>&& futs)
{
    using state_t = detail::When_all_state<Fut>;

    auto s = std::make_shared<state_t>(std::move(futs));

    return s->wait();
}

}