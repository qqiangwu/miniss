#pragma once

#include <cerrno>
#include <system_error>

namespace miniss {

inline void throw_system_error_if(const bool expr)
{
    if (expr) {
        throw std::system_error(errno, std::system_category());
    }
}

inline void throw_system_error_if(const bool expr, const char* what)
{
    if (expr) {
        throw std::system_error(errno, std::system_category(), what);
    }
}

inline const char* current_exception_message()
try {
    throw;
} catch (const std::exception& e) {
    return e.what();
} catch (...) {
    return "unknown error";
}

}