#pragma once

namespace miniss {

class Poller {
public:
    virtual ~Poller() = default;

    virtual bool poll() = 0;
    virtual bool pure_poll() { return poll(); }
};

}