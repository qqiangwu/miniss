#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>
#include <span>
#include <utility>
#include "miniss/future.h"

namespace miniss {

class CPU;

class File {
public:
    ~File();

    File(File&) = delete;
    File(File&& other) noexcept
        : cpu_(std::exchange(other.cpu_, nullptr)),
          fd_(std::exchange(other.fd_, -1))
    {
    }

    File& operator=(File&) = delete;
    File& operator=(File&& other) noexcept
    {
        std::swap(cpu_, other.cpu_);
        std::swap(fd_, other.fd_);

        return *this;
    }

    future<struct stat> stat() const;

    future<std::uint64_t> size() const;

    future<std::uint64_t> dma_write(uint64_t pos, std::span<const std::byte> bytes);
    future<std::uint64_t> dma_read(uint64_t pos, std::span<std::byte> bytes);

    future<> flush();
    future<> close();

private:
    friend CPU;

    File(CPU* cpu, int fd)
        : cpu_(cpu), fd_(fd)
    {}

private:
    CPU* cpu_;
    int fd_ = -1;
};

}