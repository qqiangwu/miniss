#pragma once

#include "miniss/configuration.h"

namespace miniss {

class App {
public:
    explicit App(const Configuration& conf) : conf_(conf)
    {
    }

    void run(Task&& task);

private:
    Configuration conf_;
};

}