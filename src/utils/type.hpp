#pragma once

#include <cstdint>
#include <format>
#include <iosfwd>
#include <sstream>

enum class Type : std::uint8_t {
    BOOL,
    I32,
    VOID,
};

std::ostream& operator<<(std::ostream& os, Type type);

template <>
struct std::formatter<Type> : std::formatter<std::string> {
    auto format(const Type& type, std::format_context& ctx) const {
        std::stringstream ss;
        ss << type;
        return std::formatter<std::string>::format(std::move(ss).str(), ctx);
    }
};
