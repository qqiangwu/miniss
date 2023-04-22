#include "miniss/io/file.h"
#include "miniss/cpu.h"
#include "miniss/io/file_io.h"
#include <cerrno>
#include <cstring>
#include <spdlog/spdlog.h>

using namespace miniss;

File::~File()
{
    if (fd_ == -1) {
        return;
    }

    close().handle_exception(
        [](std::exception_ptr e) { spdlog::error("close file failed: {}", std::strerror(errno)); });
}

future<struct stat> File::stat() const
{
    assert(fd_ != -1);
    assert(cpu_);

    return cpu_->submit_syscall([fd = fd_] {
        struct stat st;
        auto ret = ::fstat(fd, &st);
        return wrap_syscall(ret, st);
    });
}

future<std::uint64_t> File::size() const
{
    assert(fd_ != -1);
    assert(cpu_);

    return cpu_->submit_syscall([fd = fd_]() {
        auto r = ::lseek(fd, 0, SEEK_END);
        throw_system_error_if(r < 0);

        return wrap_syscall(std::uint64_t(r));
    });
}

future<std::uint64_t> File::dma_write(uint64_t pos, std::span<const std::byte> bytes)
{
    assert(fd_ >= 0);
    return file_io_->submit_write(fd_, pos, bytes);
}

future<std::uint64_t> File::dma_read(uint64_t pos, std::span<std::byte> bytes)
{
    assert(fd_ >= 0);
    return file_io_->submit_read(fd_, pos, bytes);
}

future<> File::flush()
{
    assert(fd_ != -1);
    assert(cpu_);

    return cpu_->submit_syscall([fd = fd_] { return ::fdatasync(fd); });
}

future<> File::close()
{
    if (fd_ == -1) {
        return make_ready_future<>();
    }

    auto fd = std::exchange(fd_, -1);

    return cpu_->submit_syscall([fd]() { return ::close(fd); });
}