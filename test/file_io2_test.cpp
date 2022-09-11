#include <fcntl.h>
#include <filesystem>
#include <vector>
#include <string_view>
#include <chrono>
#include <thread>
#include <fmt/core.h>
#include <scope_guard.hpp>
#include <gtest/gtest.h>
#include "miniss/io/file_io.h"
#include "miniss/cpu.h"
#include "miniss/app.h"
#include "miniss/seastar/do_with.h"

using namespace miniss;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

constexpr std::string_view kTestPath = "miniss.file_io.test";
constexpr std::uintmax_t kTestFileSize = 1024 * 16;

int main()
{
    const auto p = fs::temp_directory_path() / kTestPath;
    SCOPE_EXIT {
        fs::remove(p);
    };

    fs::remove(p);

    alignas(512) std::array<std::byte, kTestFileSize> buf;
    alignas(512) std::array<std::byte, kTestFileSize> buf2;

    Configuration conf;
    conf.cpu_count = 1;

    std::vector<int> values(conf.cpu_count);
    App app(conf);
    app.run([&]{
        return this_cpu()->open_file(p, O_RDWR | O_CREAT).then_wrapped([&](auto&& f){
            EXPECT_TRUE(!f.failed());

            return do_with(f.get0(), [&](auto& file){
                return file.dma_read(0, buf).then_wrapped([](auto&& f){
                    EXPECT_TRUE(!f.failed());
                    EXPECT_EQ(f.get0(), 0);
                }).then([&]{
                    std::fill(buf.begin(), buf.end(), std::byte{'x'});

                    return file.dma_write(0, buf);
                }).then_wrapped([&](auto&& f){
                    EXPECT_TRUE(!f.failed());
                    EXPECT_EQ(f.get0(), kTestFileSize);

                    return file.dma_read(0, buf2);
                }).then_wrapped([&](auto&& f){
                    EXPECT_TRUE(!f.failed());
                    EXPECT_EQ(f.get0(), kTestFileSize);

                    EXPECT_EQ(buf, buf2);
                });
            });
        }).then_wrapped([](auto&& f){
            EXPECT_TRUE(!f.failed());
            return 0;
        });
    });
}
