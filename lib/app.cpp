#include "miniss/app.h"
#include "miniss/cpu.h"
#include "miniss/os.h"

using namespace miniss;

int App::run(std::function<future<int>()>&& task)
{
    os()->init(conf_);

    this_cpu()->schedule([task = std::move(task)] {
        futurize_apply(std::move(task)).then_wrapped([](auto&& f) {
            if (f.failed()) {
                os()->exit(-1);
            } else {
                os()->exit(f.get0());
            }
        });
    });

    return this_cpu()->run();
}