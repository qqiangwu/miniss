#include "miniss/io/file_io.h"
#include "miniss/task.h"
#include <array>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <nonstd/scope.hpp>
#include <string_view>
#include <thread>
#include <vector>

using namespace miniss;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

namespace miniss {

__attribute__((weak)) void schedule(std::unique_ptr<task> t) { t->run(); }

__attribute__((weak)) void schedule_urgent(std::unique_ptr<task> t) { t->run(); }

}

constexpr std::string_view kTestPath = "miniss.file_io.test";
constexpr std::uintmax_t kTestFileSize = 1024 * 16;

int main()
{
    const auto p = fs::temp_directory_path() / kTestPath;
    const auto guard = nonstd::make_scope_exit([&] { fs::remove(p); });

    fs::remove(p);

    File_io fio;

    int fd = ::open(p.c_str(), O_RDWR | O_CREAT | O_DIRECT, 0644);
    EXPECT_TRUE(fd >= 0);

    alignas(512) std::array<std::byte, kTestFileSize> buf;
    alignas(512) std::array<std::byte, kTestFileSize> buf2;
    auto f = fio.submit_read(fd, 0, buf)
                 .then_wrapped([](auto&& f) {
                     EXPECT_TRUE(!f.failed());
                     EXPECT_EQ(f.get0(), 0);
                 })
                 .then([&] {
                     std::fill(buf.begin(), buf.end(), std::byte { 'x' });

                     return fio.submit_write(fd, 0, buf);
                 })
                 .then_wrapped([&](auto&& f) {
                     EXPECT_TRUE(!f.failed());
                     EXPECT_EQ(f.get0(), kTestFileSize);

                     return fio.submit_read(fd, 0, buf2);
                 })
                 .then_wrapped([&](auto&& f) {
                     EXPECT_TRUE(!f.failed());
                     EXPECT_EQ(f.get0(), kTestFileSize);

                     EXPECT_EQ(buf, buf2);
                 });

    using namespace std::chrono_literals;
    const auto deadline = std::chrono::system_clock::now() + 10s;
    while (std::chrono::system_clock::now() < deadline) {
        fio.reap_io();

        if (f.available()) {
            break;
        }

        std::this_thread::sleep_for(10ms);
    }

    EXPECT_TRUE(f.available());
    EXPECT_TRUE(!f.failed());
}
