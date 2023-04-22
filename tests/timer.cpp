#include "miniss/app.h"
#include "miniss/os.h"
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/core.h>

using namespace miniss;
using namespace std::chrono;
using namespace std::chrono_literals;

int main()
{
    Configuration conf;
    conf.cpu_count = 1;

    constexpr auto kInterval = 2s;
    const auto started = Clock_type::now();

    App app(conf);
    app.run([=] {
        promise<int> pr;
        auto fut = pr.get_future();

        this_cpu()->schedule_after(kInterval, [=, pr = std::move(pr)]() mutable {
            const auto end = Clock_type::now();
            fmt::print("time ellapsed: {}\n", duration_cast<milliseconds>(end - started));

            pr.set_value(0);
        });

        return fut;
    });
}
