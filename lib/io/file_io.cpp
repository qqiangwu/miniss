#include "miniss/io/file_io.h"
#include <cerrno>
#include <cstring>
#include <linux/aio_abi.h>
#include <spdlog/spdlog.h>

using namespace miniss;

File_io::File_io()
{
    auto r = ::syscall(__NR_io_setup, max_io, &aio_ctx_);
    throw_system_error_if(r < 0);
}

File_io::~File_io()
{
    auto r = ::syscall(__NR_io_destroy, aio_ctx_);
    if (r < 0) {
        spdlog::error("destroy aio failed: {}", std::strerror(errno));
    }
}

future<std::uint64_t> File_io::submit_write(int fd, uint64_t pos, std::span<const std::byte> bytes)
{
    iocb io;
    ::memset(&io, 0, sizeof(io));

    io.aio_lio_opcode = IOCB_CMD_PWRITE;
    io.aio_fildes = fd;
    io.aio_buf = reinterpret_cast<std::uintptr_t>(bytes.data());
    io.aio_nbytes = bytes.size();
    io.aio_offset = pos;

    return submit_io_(io).then([](io_event ev) {
        throw_kernel_error(long(ev.res));
        return make_ready_future<std::uint64_t>(std::uint64_t(ev.res));
    });
}

future<std::uint64_t> File_io::submit_read(int fd, uint64_t pos, std::span<std::byte> bytes)
{
    iocb io;
    ::memset(&io, 0, sizeof(io));

    io.aio_lio_opcode = IOCB_CMD_PREAD;
    io.aio_fildes = fd;
    io.aio_buf = reinterpret_cast<std::uintptr_t>(bytes.data());
    io.aio_nbytes = bytes.size();
    io.aio_offset = pos;

    return submit_io_(io).then([](io_event ev) {
        throw_kernel_error(long(ev.res));
        return make_ready_future<std::uint64_t>(std::uint64_t(ev.res));
    });
}

future<io_event> File_io::submit_io_(iocb io)
{
    io.aio_resfd = aio_eventfd_.get_fd();
    io.aio_flags |= IOCB_FLAG_RESFD;

    return aio_available_.wait().then([io, this]() mutable { return submit_io_impl_(&io); });
}

future<io_event> File_io::submit_io_impl_(iocb* io)
{
    auto pr = std::make_unique<promise<io_event>>();
    auto fut = pr->get_future();

    io->aio_data = reinterpret_cast<std::uintptr_t>(pr.get());
    auto r = ::syscall(__NR_io_submit, aio_ctx_, 1, &io);
    throw_system_error_if(r < 0, "io_submit failed");

    pr.release();
    return fut;
}

bool File_io::reap_io()
{
    io_event ev[max_io];
    struct timespec timeout = { 0, 0 };
    auto n = ::syscall(__NR_io_getevents, aio_ctx_, 1, max_io, ev, &timeout);
    throw_system_error_if(n < 0, "io_getevents failed");

    for (size_t i = 0; i < size_t(n); ++i) {
        auto pr = reinterpret_cast<promise<io_event>*>(ev[i].data);
        pr->set_value(ev[i]);
        delete pr;
    }

    aio_available_.release(n);
    return n > 0;
}