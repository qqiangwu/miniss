#include "miniss/os.h"
#include "miniss/cpu.h"
#include "miniss/app.h"

using namespace miniss;

void App::run(Task&& task)
{
    os()->init(conf_);
    this_cpu()->schedule(std::move(task));
    this_cpu()->run();
}