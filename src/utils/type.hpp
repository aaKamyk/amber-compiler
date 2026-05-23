#pragma once

#include <cstdint>
#include <string>

#include "fmt/base.h"

enum class Type : std::uint8_t {
    BOOL,
    I32,
    VOID,
};

std::string to_string(Type type);

template <>
struct fmt::formatter<Type> : formatter<string_view> {
    auto format(const Type type, format_context& ctx) const {
        const string_view name = ::to_string(type);
        return formatter<string_view>::format(name, ctx);
    }
};