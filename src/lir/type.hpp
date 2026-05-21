#pragma once

#include <iosfwd>

#include "utils/type.hpp"

namespace lir {

enum class Type {
    // integer types
    I1,
    I8,
    I16,
    I32,
    I64,
    // special
    VOID
};

Type type_from(::Type);

std::ostream& operator<<(std::ostream& os, Type type);

}  // namespace lir
