#pragma once

#include <cstddef>
#include <deque>
#include <boost/noncopyable.hpp>
#include "miniss/future.h"

namespace miniss {

class Semaphore : private boost::noncopyable {
    struct Entry {
        std::size_t tokens;
        promise<> pr;
    };

    std::size_t tokens_;
    std::deque<Entry> waiting_list_;

public:
    explicit Semaphore(std::size_t token)
        : tokens_(token)
    {
    }

    future<> wait(std::size_t tokens = 1);

    void release(std::size_t tokens = 1);
};

}