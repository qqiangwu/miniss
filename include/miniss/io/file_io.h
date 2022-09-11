#pragma once

#include <linux/aio_abi.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <boost/noncopyable.hpp>
#include "miniss/future.h"
#include "miniss/util/eventfd.h"
#include "miniss/util/semaphore.h"

namespace miniss {

class CPU;

class File_io : public boost::noncopyable {
public:
    File_io();
    ~File_io();

    future<std::uint64_t> submit_write(int fd, uint64_t pos, std::span<const std::byte> bytes);
    future<std::uint64_t> submit_read(int fd, uint64_t pos, std::span<std::byte> bytes);

    // called by poller to reap io
    bool reap_io();

    int get_eventfd() const
    {
        return aio_eventfd_.get_fd();
    }

private:
    future<io_event> submit_io_(iocb io);
    future<io_event> submit_io_impl_(iocb* io);

private:
    enum { max_io = 128 };

    Semaphore aio_available_ { max_io };
    Eventfd aio_eventfd_;
    aio_context_t aio_ctx_;
};

}