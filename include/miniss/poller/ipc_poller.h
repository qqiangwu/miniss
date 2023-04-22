#pragma once

#include "miniss/poller.h"
#include <functional>
#include <map>

namespace miniss {

class Ipc_poller : public Poller {
public:
    explicit Ipc_poller(unsigned cpu_id)
        : cpu_id_(cpu_id)
    {
    }

    bool poll() override;
    bool pure_poll() override;

private:
    unsigned cpu_id_;
};

}