#include "miniss/io/syscall.h"
#include "miniss/cpu.h"
#include <array>

using namespace miniss;

Syscall_runner::Syscall_runner(CPU& cpu)
    : cpu_(cpu)
    , worker_([this](auto token) { worker_run_(token); })
{
}

Syscall_runner::~Syscall_runner()
{
    worker_.request_stop();
    interrupter_.signal(1);
}

bool Syscall_runner::poll_results()
{
    std::array<Work_item*, queue_size> buf {};

    const auto nr = result_queue_.pop(buf.data(), buf.size());
    std::for_each_n(buf.begin(), nr, [this](Work_item* work) {
        std::unique_ptr<Work_item> p(work);
        p->complete();
    });

    queue_has_room_.release(nr);

    return nr > 0;
}

void Syscall_runner::submit_(Work_item* work)
{
    queue_has_room_.wait().then([this, work] {
        call_queue_.push(work);
        interrupter_.signal();
    });
}

void Syscall_runner::respond_(Work_item* work) { result_queue_.push(work); }

void Syscall_runner::worker_run_(std::stop_token token)
{
    while (!token.stop_requested()) {
        interrupter_.wait();

        if (token.stop_requested()) {
            break;
        }

        while (true) {
            auto nr = call_queue_.consume_all([this](Work_item* item) {
                item->process();
                respond_(item);
            });

            if (nr == 0) {
                break;
            }

            cpu_.maybe_wakeup();
        }
    }
}