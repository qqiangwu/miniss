#include "miniss/io/syscall.h"
#include "miniss/poller/syscall_poller.h"

using namespace miniss;

bool Syscall_poller::poll()
{
    return runner_.poll_results();
}