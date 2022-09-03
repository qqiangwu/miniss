#pragma once

#include <vector>
#include <thread>
#include <memory>
#include "miniss/cpu.h"

namespace miniss {

class OS {
public:
    void init(const Configuration& conf);
    void exit(int code);

private:
    void start_cpu_(int cpu_id, const Configuration& conf);

private:
    std::vector<std::thread> threads_;
    std::vector<std::unique_ptr<CPU>> cpus_;
};

inline OS* os()
{
    static OS os_instance;
    return &os_instance;
}

}