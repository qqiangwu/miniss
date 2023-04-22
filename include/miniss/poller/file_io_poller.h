#pragma once

#include "miniss/poller.h"

namespace miniss {

class File_io;

class File_io_poller : public Poller {
public:
    explicit File_io_poller(File_io& fio)
        : file_io_(fio)
    {
    }

    bool poll() override;

private:
    File_io& file_io_;
};

}