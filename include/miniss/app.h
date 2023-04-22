#pragma once

#include "miniss/configuration.h"
#include "miniss/future.h"

namespace miniss {

class App {
public:
    explicit App(const Configuration& conf)
        : conf_(conf)
    {
    }

    int run(std::function<future<int>()>&& task);

private:
    Configuration conf_;
};

}