#pragma once

#include <iostream>
#include <source_location>
#include <string_view>

namespace lib {

constexpr void assert(
    const bool condition,
    const std::string_view message = "expression evaluates to false",
    const std::source_location location = std::source_location::current()
) {
#ifdef ACT_DEBUG
    if (!condition) {
        std::cerr << "Assertion failed: " << message << '\n'
                  << "File: " << location.file_name() << '(' << location.line() << ':'
                  << location.column() << ")\n"
                  << "Function: " << location.function_name() << std::endl;
        std::abort();
    }
#else
#endif
}

}  // namespace lib