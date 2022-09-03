#include <cassert>
#include "miniss/os.h"
#include "miniss/poller/ipc_poller.h"

using namespace miniss;

bool Ipc_poller::poll()
{
    return os()->poll_queues();
}

bool Ipc_poller::pure_poll()
{
    assert(!"not implement");
    return false;
}