#pragma once

#include <cstdint>
#include <iosfwd>

enum class Type : std::uint8_t {
    INT,
    VOID,
};

std::ostream& operator<<(std::ostream& os, Type type);
