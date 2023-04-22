#include "miniss/task.h"
#include "miniss/cpu.h"

void miniss::schedule(std::unique_ptr<task> t) { this_cpu()->schedule(std::move(t)); }

void miniss::schedule_urgent(std::unique_ptr<task> t) { this_cpu()->schedule(std::move(t)); }