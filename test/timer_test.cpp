#include <chrono>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include "miniss/os.h"
#include "miniss/app.h"

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
    app.run([=]{
        this_cpu()->schedule_after(kInterval, [=]{
            const auto end = Clock_type::now();
            fmt::print("time ellapsed: {}\n", duration_cast<milliseconds>(end - started));

            os()->exit(0);
        });
    });
}
