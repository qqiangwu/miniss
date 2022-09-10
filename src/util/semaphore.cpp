#include "miniss/util/semaphore.h"

using namespace miniss;

future<> Semaphore::wait(std::size_t tokens)
{
    if (tokens < tokens_ && waiting_list_.empty()) {
        tokens_ -= tokens;
        return make_ready_future<>();
    }

    promise<> pr;
    auto fut = pr.get_future();

    waiting_list_.push_back(Entry(tokens, std::move(pr)));

    return fut;
}

void Semaphore::release(std::size_t tokens)
{
    tokens_ += tokens;

    if (waiting_list_.empty() || waiting_list_.front().tokens > tokens_) {
        return;
    }

    auto& entry = waiting_list_.front();

    tokens_ -= entry.tokens;
    entry.pr.set_value();

    waiting_list_.pop_front();
}