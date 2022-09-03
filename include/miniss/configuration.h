#pragma once

#include <chrono>
#include <stdexcept>
#include <functional>

namespace miniss {

class Invalid_config_error : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

struct Configuration {
    unsigned cpu_count = 0;
};

using Clock_type = std::chrono::steady_clock;
using Task = std::function<void()>;

}