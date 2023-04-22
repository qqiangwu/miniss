#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <fmt/core.h>
#include <nonstd/scope.hpp>
#include <gtest/gtest.h>
#include "miniss/cpu.h"
#include "miniss/app.h"
#include "miniss/future_util.h"

using namespace miniss;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

constexpr std::string_view kNonExistPath = "miniss.file_meta.test0";
constexpr std::string_view kTestPath = "miniss.file_meta.test1";
constexpr std::uintmax_t kTestFileSize = 1024 * 16;

int main()
{
    const auto p1 = fs::temp_directory_path() / kNonExistPath;
    const auto p2 = fs::temp_directory_path() / kTestPath;
    const auto guard = nonstd::make_scope_exit([&] {
        fs::remove(p1);
        fs::remove(p2);
    });

    fs::remove(p1);
    fs::remove(p2);

    std::ofstream(p2).put('\0');
    fs::resize_file(p2, kTestFileSize);

    Configuration conf;
    conf.cpu_count = 1;

    std::vector<int> values(conf.cpu_count);
    App app(conf);
    app.run([&]{
        auto cpu = this_cpu();

        auto f1 = cpu->open_file(p1, O_RDONLY).then_wrapped([](auto&& f){
            fmt::print("open failed\n");

            EXPECT_TRUE(f.failed());
            EXPECT_THROW(f.get(), std::system_error);
        });

        auto f2 = cpu->open_file(p2, O_RDONLY).then([](File file){
            return file.size();
        }).then([](auto size){
            fmt::print("filesize: {} - {}\n", size, kTestFileSize);

            EXPECT_EQ(size, kTestFileSize);
        });

        std::vector<future<>> futs;
        futs.push_back(std::move(f1));
        futs.push_back(std::move(f2));
        return when_all(std::move(futs)).then([](auto&& f){
            return make_ready_future<int>(0);
        });
    });
}
