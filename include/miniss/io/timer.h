#pragma once

#include "miniss/configuration.h"
#include "miniss/task.h"
#include <chrono>
#include <map>
#include <time.h>
#include <vector>

namespace miniss {

class CPU;

class Timer_service {
public:
    explicit Timer_service(CPU& cpu);

    ~Timer_service();

    void add_timer(Clock_type::duration, std::unique_ptr<task> t);
    void complete_timers();

private:
    void rearm_timer_();

private:
    CPU& cpu_;

    timer_t physical_timer_ = {};
    std::map<Clock_type::time_point, std::vector<std::unique_ptr<task>>> timers_;
};

}