#include "assert.hpp"
#include <fmt/format.h>
#include <cstdlib> // std::abort


void
handle_failed_debug_assertion(
        char const* msg, char const* func, char const* file, int line) noexcept
{
    try {
        fmt::print(stderr, "debug assertion failure in {} ({}:{}): {}\n", func, file, line, msg);
    } catch (...) {}

    std::abort();
}
