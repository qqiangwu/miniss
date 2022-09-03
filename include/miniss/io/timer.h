#pragma once

#include <time.h>
#include <chrono>
#include <map>
#include <vector>
#include "miniss/configuration.h"

namespace miniss {

class CPU;

class Timer_service {
public:
    explicit Timer_service(CPU& cpu);

    ~Timer_service();

    void add_timer(Clock_type::duration, Task&& task);
    void complete_timers();

private:
    void rearm_timer_();

private:
    CPU& cpu_;

    timer_t physical_timer_ = {};
    std::map<Clock_type::time_point, std::vector<Task>> timers_;
};

}