#pragma once

#include "miniss/future.h"
#include "miniss/util/eventfd.h"
#include "miniss/util/semaphore.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/noncopyable.hpp>
#include <memory>
#include <system_error>
#include <thread>
#include <type_traits>
#include <variant>

namespace miniss {

template <typename Extra> struct syscall_result {
    int error;
    Extra extra;

    void throw_if_error()
    {
        if (error == -1) {
            throw std::system_error(error, std::system_category());
        }
    }
};

template <typename Extra> inline syscall_result<Extra> wrap_syscall(const Extra& extra)
{
    syscall_result<Extra> r;

    r.error = 0;
    r.extra = extra;

    return r;
}

template <typename Extra> inline syscall_result<Extra> wrap_syscall(int result, const Extra& extra)
{
    syscall_result<Extra> r;

    r.error = result < 0 ? errno : 0;
    r.extra = extra;

    return r;
}

// @fixme predicate on T is syscall_result<E> is somehow hard
template <class T>
concept IsSyscallResult = requires(T t)
{
    {
        t.error
        } -> std::convertible_to<int>;
    { t.extra };
    { t.throw_if_error() };
}
|| std::is_same_v<T, int>;

class CPU;

namespace detail {

    template <class T> struct Result_selector {
        using value_type = decltype(T().extra);
        using result_holder = std::variant<std::monostate, value_type, std::exception_ptr>;
    };

    template <> struct Result_selector<int> {
        using value_type = void;
        using result_holder = std::variant<std::monostate, std::exception_ptr>;
    };

}

// run syscall in background thread
class Syscall_runner : private boost::noncopyable {
    struct Work_item : private boost::noncopyable {
        virtual ~Work_item() = default;

        virtual void process() = 0;
        virtual void complete() = 0;
    };

    template <class Fn> struct Syscall_work_item final : public Work_item {
        using result_type = std::result_of_t<Fn()>;
        using result_selector = detail::Result_selector<result_type>;
        using value_type = typename result_selector::value_type;
        using result_holder = typename result_selector::result_holder;

        enum { kNoPayload = std::is_same_v<result_type, int> };

        Fn fn_;
        result_holder ret_;
        promise<value_type> promise_;

        explicit Syscall_work_item(Fn&& fn)
            : fn_(std::move(fn))
        {
        }

        void process() override
        {
            try {
                auto r = fn_();

                if constexpr (kNoPayload) {
                    throw_system_error_if(r < 0);
                } else {
                    throw_system_error_if(r.error < 0);

                    ret_.template emplace<value_type>(std::move(r.extra));
                }
            } catch (...) {
                ret_.template emplace<std::exception_ptr>(std::current_exception());
            }
        }

        void complete() override
        {
            if constexpr (kNoPayload) {
                switch (ret_.index()) {
                case 0:
                    promise_.set_value();
                    break;

                case 1:
                    promise_.set_exception(std::move(std::get<std::exception_ptr>(ret_)));
                    break;

                default:
                    assert(!"unreachable");
                    std::abort();
                }
            } else {
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
        }
    };

public:
    explicit Syscall_runner(CPU& cpu);

    ~Syscall_runner();

    template <class Fn>
    requires IsSyscallResult<std::result_of_t<Fn()>>
    auto submit(Fn&& fn)
    {
        auto task = std::make_unique<Syscall_work_item<Fn>>(std::forward<Fn>(fn));
        auto fut = task->promise_.get_future();

        submit_(task.get());

        task.release();
        return fut;
    }

    bool poll_results();

private:
    void submit_(Work_item* work);
    void respond_(Work_item* work);

    void worker_run_(std::stop_token token);

private:
    static constexpr std::size_t queue_size = 128;

    using Spsc_queue = boost::lockfree::spsc_queue<Work_item*, boost::lockfree::capacity<queue_size>>;

    CPU& cpu_;
    Semaphore queue_has_room_ { queue_size };

    Spsc_queue call_queue_;
    Spsc_queue result_queue_;

    Eventfd interrupter_;
    std::jthread worker_;
};

}