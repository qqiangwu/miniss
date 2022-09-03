#pragma once

#include <memory>

namespace miniss {

class task {
public:
    virtual ~task() noexcept {}
    virtual void run() noexcept = 0;
};

void schedule(std::unique_ptr<task> t);
void schedule_urgent(std::unique_ptr<task> t);

template <typename Func>
class lambda_task final : public task {
    Func _func;
public:
    lambda_task(const Func& func) : _func(func) {}
    lambda_task(Func&& func) : _func(std::move(func)) {}
    virtual void run() noexcept override { _func(); }
};

template <typename Func>
inline std::unique_ptr<task> make_task(Func&& func) {
    return std::make_unique<lambda_task<Func>>(std::forward<Func>(func));
}

}