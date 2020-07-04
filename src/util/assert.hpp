#pragma once


#ifndef NDEBUG
#    define DEBUG_ASSERT(expr)                                                                   \
        do {                                                                                     \
            static_cast<void>((expr) || (handle_failed_debug_assertion("\"" #expr "\""), true)); \
        } while (0)
#else
#    define DEBUG_ASSERT(expr)
#endif

void handle_failed_debug_assertion(char const* msg, char const* func = __builtin_FUNCTION(),
        char const* file = __builtin_FILE(), int line = __builtin_LINE()) noexcept;
