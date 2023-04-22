#include "miniss/poller/file_io_poller.h"
#include "miniss/io/file_io.h"

using namespace miniss;

bool File_io_poller::poll() { return file_io_.reap_io(); }