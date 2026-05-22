#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "fmt/base.h"

enum class Type : std::uint8_t {
    BOOL,
    I32,
    VOID,
};

std::string to_string(Type type);

std::ostream& operator<<(std::ostream& os, Type type);

template <>
struct fmt::formatter<Type> {
    static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    constexpr auto format(const Type& type, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", to_string(type));
    }
};