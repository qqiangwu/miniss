#include <cstdlib>
#include <latch>
#include "miniss/os.h"

using namespace miniss;

void OS::init(const Configuration& conf)
{
    threads_.reserve(conf.cpu_count - 1);
    cpus_.resize(conf.cpu_count);

    std::latch start_latch(conf.cpu_count);
    for (size_t i = 1; i < conf.cpu_count; ++i) {
        threads_.emplace_back([&, i, this]{
            start_cpu_(i, conf);
            start_latch.arrive_and_wait();

            this_cpu()->run();
        });
    }

    start_cpu_(0, conf);
    start_latch.arrive_and_wait();
}

void OS::start_cpu_(int i, const Configuration& conf)
{
    cpus_[i] = std::make_unique<CPU>(conf, i);
    local_cpu_ = cpus_[i].get();
}

// @todo: fix me later
void OS::exit(int code)
{
    std::_Exit(code);
}