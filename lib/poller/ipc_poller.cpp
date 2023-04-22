#include "miniss/poller/ipc_poller.h"
#include "miniss/os.h"
#include <cassert>

using namespace miniss;

bool Ipc_poller::poll() { return os()->poll_queues(); }

bool Ipc_poller::pure_poll()
{
    assert(!"not implement");
    return false;
}