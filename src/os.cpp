#include <cassert>
#include <cstdlib>
#include <latch>
#include "miniss/os.h"

using namespace miniss;

void OS::init(const Configuration& conf)
{
    conf_ = conf;
    threads_.reserve(conf.cpu_count - 1);
    cpus_.resize(conf.cpu_count);

    // avoid premature destruction before wait are done in other threads
    static std::latch cpu_constructed(conf.cpu_count);
    static std::latch queue_constructed(conf.cpu_count);
    static std::latch started(conf.cpu_count);

    for (size_t i = 1; i < conf.cpu_count; ++i) {
        threads_.emplace_back([&, i, this]{
            start_cpu_(i, conf);
            cpu_constructed.arrive_and_wait();
            queue_constructed.arrive_and_wait();
            started.arrive_and_wait();

            this_cpu()->run();
        });
    }

    start_cpu_(0, conf);
    cpu_constructed.arrive_and_wait();

    queues_ = new Cross_cpu_queue*[conf.cpu_count];
    for (std::size_t i = 0; i < conf.cpu_count; ++i) {
        assert(cpus_[i]);

        queues_[i] = reinterpret_cast<Cross_cpu_queue*>(operator new[](sizeof(Cross_cpu_queue) * conf.cpu_count));
        for (std::size_t j = 0; j < conf.cpu_count; ++j) {
            new(&queues_[i][j]) Cross_cpu_queue(*cpus_[i], *cpus_[j]);
        }
    }

    queue_constructed.arrive_and_wait();
    started.arrive_and_wait();
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

bool OS::poll_queues()
{
    const auto current_cpu = this_cpu()->cpu_id();

    bool r = false;
    for (size_t i = 0; i < conf_.cpu_count; ++i) {
        if (i == current_cpu) {
            continue;
        }

        r |= queues_[current_cpu][i].flush_tx();
        r |= queues_[current_cpu][i].poll_rx();

        r |= queues_[i][current_cpu].flush_rx();
        r |= queues_[i][current_cpu].poll_tx();
    }

    return r;
}