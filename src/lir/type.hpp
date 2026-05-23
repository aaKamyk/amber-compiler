#pragma once

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

}  // namespace lir

std::string to_string(lir::Type type);

template <>
struct fmt::formatter<lir::Type> : formatter<string_view> {
    auto format(const lir::Type type, format_context& ctx) const {
        const string_view name = ::to_string(type);
        return formatter<string_view>::format(name, ctx);
    }
};