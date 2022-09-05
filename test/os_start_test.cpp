#include <fmt/core.h>
#include "miniss/os.h"
#include "miniss/app.h"

using namespace miniss;

int main()
{
    Configuration conf;
    conf.cpu_count = 4;

    App app(conf);
    app.run([]{
        fmt::print("I am test!\n");
        return make_ready_future<int>();
    });
}
