#pragma once

#include "miniss/cpu.h"
#include "miniss/cross_cpu_queue.h"
#include "miniss/future.h"
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace miniss {

class OS {
public:
    void init(const Configuration& conf);
    void exit(int code);

    template <class Fn> futurize_t<std::result_of_t<Fn()>> submit_to(unsigned cpu_id, Fn&& fn)
    {
        if (this_cpu()->cpu_id() != cpu_id) {
            return queues_[this_cpu()->cpu_id()][cpu_id].submit(std::forward<Fn>(fn));
        }

        using ret_type = std::result_of_t<Fn()>;
        return futurize<ret_type>::apply(std::forward<Fn>(fn));
    }

    bool poll_queues();

private:
    void start_cpu_(int cpu_id, const Configuration& conf);

private:
    Configuration conf_;
    std::vector<std::thread> threads_;
    std::vector<std::unique_ptr<CPU>> cpus_;
    Cross_cpu_queue** queues_ = nullptr;
};

inline OS* os()
{
    static OS os_instance;
    return &os_instance;
}

}