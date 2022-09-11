#pragma once

#include <cerrno>
#include <type_traits>
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

template <typename T>
inline void throw_kernel_error(T r)
{
    static_assert(std::is_signed<T>::value, "kernel error variables must be signed");
    if (r < 0) {
        throw std::system_error(-r, std::system_category());
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