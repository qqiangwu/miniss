#pragma once

#include <functional>
#include <map>
#include "miniss/poller.h"

namespace miniss {

class Syscall_runner;

class Syscall_poller: public Poller {
public:
    explicit Syscall_poller(Syscall_runner& runner)
        : runner_(runner)
    {}

    bool poll() override;

private:
    Syscall_runner& runner_;
};

}