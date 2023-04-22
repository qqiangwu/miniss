#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <new>
#include <fmt/core.h>
#include <nonstd/scope.hpp>
#include <gtest/gtest.h>
#include "miniss/io/file_io.h"
#include "miniss/cpu.h"
#include "miniss/app.h"
#include "miniss/enable_coroutine.h"

using namespace miniss;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

constexpr std::string_view kTestPath = "miniss.file_io.test";
constexpr std::uintmax_t kTestFileSize = 1024 * 16;

int main()
{
    const auto p = fs::temp_directory_path() / kTestPath;
    const auto guard = nonstd::make_scope_exit([&] {
        fs::remove(p);
    });

    fs::remove(p);

    Configuration conf;
    conf.cpu_count = 1;

    std::vector<int> values(conf.cpu_count);
    App app(conf);
    app.run([&]() -> future<int> {
        std::unique_ptr<std::byte[]> b1(new(std::align_val_t(4096)) std::byte[kTestFileSize]);
        std::unique_ptr<std::byte[]> b2(new(std::align_val_t(4096)) std::byte[kTestFileSize]);

        std::span<std::byte> buf1(b1.get(), kTestFileSize);
        std::span<std::byte> buf2(b2.get(), kTestFileSize);

        auto file = co_await this_cpu()->open_file(p, O_RDWR | O_CREAT);

        auto nr = co_await file.dma_read(0, buf1);
        EXPECT_EQ(nr, 0);

        std::fill_n(buf1.begin(), buf1.size(), std::byte{'x'});
        nr = co_await file.dma_write(0, buf1);
        EXPECT_EQ(nr, kTestFileSize);

        nr = co_await file.dma_read(0, buf2);
        EXPECT_EQ(nr, kTestFileSize);
        EXPECT_TRUE(std::equal(buf1.begin(), buf1.end(), buf2.begin()));

        co_return 0;
    });
}
