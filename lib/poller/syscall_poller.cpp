#include "miniss/poller/syscall_poller.h"
#include "miniss/io/syscall.h"

using namespace miniss;

bool Syscall_poller::poll() { return runner_.poll_results(); }