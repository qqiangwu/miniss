#pragma once

#include <coroutine>
#include <boost/noncopyable.hpp>
#include "miniss/future.h"

namespace miniss::detail {

template <class... T>
struct coroutine_traits_base {
    class promise_type : private boost::noncopyable {
        promise<T...> pr_;

    public:
        future<T...> get_return_object() noexcept
        {
            return pr_.get_future();
        }

        template <class... U>
        void return_value(U&&... args) noexcept
        {
            pr_.set_value(std::forward<U>(args)...);
        }

        void unhandled_exception() noexcept
        {
            pr_.set_exception(std::current_exception());
        }

        auto initial_suspend() const noexcept
        {
            return std::suspend_never{};
        }

        auto final_suspend() const noexcept
        {
            return std::suspend_never{};
        }
    };
};

template <>
struct coroutine_traits_base<> {
    class promise_type : private boost::noncopyable {
        promise<> pr_;

    public:
        future<> get_return_object() noexcept
        {
            return pr_.get_future();
        }

        void return_void() noexcept
        {
            pr_.set_value();
        }

        void unhandled_exception() noexcept
        {
            pr_.set_exception(std::current_exception());
        }

        auto initial_suspend() const noexcept
        {
            return std::suspend_never{};
        }

        auto final_suspend() const noexcept
        {
            return std::suspend_never{};
        }
    };
};

template <class... T>
class future_awaiter : private boost::noncopyable {
    future<T...> fut_;

public:
    explicit future_awaiter(future<T...>&& fut)
        : fut_(std::move(fut))
    {
    }

    bool await_ready() noexcept
    {
        return fut_.available();
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept
    {
        fut_.then_wrapped([this, handle](auto&& f){
            fut_ = std::move(f);
            handle.resume();
        });
    }

    auto await_resume()
    {
        if constexpr (sizeof...(T) <= 1) {
            return fut_.get0();
        } else {
            return fut_.get();
        }
    }
};

}

template <class... T>
auto operator co_await(future<T...> f) noexcept
{
    return miniss::detail::future_awaiter<T...>{std::move(f)};
}

namespace std {

template <class... T, class... Args>
struct coroutine_traits<future<T...>, Args...> : miniss::detail::coroutine_traits_base<T...>
{
};

}