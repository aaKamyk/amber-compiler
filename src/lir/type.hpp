#pragma once

#include <iosfwd>

#include "utils/type.hpp"

namespace lir {

enum class Type { I32, VOID };

Type type_from(::Type);

std::ostream& operator<<(std::ostream& os, Type type);

}  // namespace lir
