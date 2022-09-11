#pragma once

#include <sys/eventfd.h>
#include <cstdint>
#include <system_error>
#include <boost/noncopyable.hpp>
#include "miniss/util.h"

namespace miniss {

class Eventfd : private boost::noncopyable {
    int fd_;

public:
    Eventfd() : fd_(::eventfd(0, EFD_CLOEXEC))
    {
        throw_system_error_if(fd_ < 0, "create eventfd failed");
    }

    ~Eventfd()
    {
        if (fd_ < 0) {
            return;
        }

        ::close(fd_);
        fd_ = -1;
    }

    int get_fd() const
    {
        return fd_;
    }

    void signal(std::uint64_t count = 1)
    {
        int r = ::write(fd_, &count, sizeof(count));
        throw_system_error_if(r < 0, "write to eventfd failed");
    }

    std::uint64_t wait()
    {
        uint64_t count;
        int r = ::read(fd_, &count, sizeof(count));
        throw_system_error_if(r < 0, "read eventfd failed");

        return count;
    }
};

}