#include "miniss/app.h"
#include "miniss/future_util.h"
#include "miniss/os.h"
#include <chrono>
#include <fmt/core.h>
#include <fmt/ranges.h>

using namespace miniss;
using namespace std::chrono_literals;

int main()
{
    Configuration conf;
    conf.cpu_count = 4;

    std::vector<int> values(conf.cpu_count);
    App app(conf);
    app.run([&] {
        std::vector<future<>> tasks;

        for (size_t i = 0; i < conf.cpu_count; ++i) {
            auto f = os()->submit_to(i, [&] {
                fmt::print("call cpu {}\n", this_cpu()->cpu_id());
                values[this_cpu()->cpu_id()] = this_cpu()->cpu_id() + conf.cpu_count;
            });

            tasks.push_back(std::move(f));
        }

        return when_all(std::move(tasks)).then([&](auto&& f) {
            fmt::print("done with {}\n", values);
            return make_ready_future<int>(0);
        });
    });
}
