#pragma once

#include <cstdint>

#include "fmt/core.h"

namespace lir {

enum class BinaryIntegerOperator : uint8_t {
    ADD,
    SUB,
    MUL,
    UDIV,  /// unsigned integer division
    SDIV,  /// signed integer division
    UREM,  /// unsigned integer division remainder
    SREM,  /// signed integer division remainder
};

enum class BinaryFloatingPointOperator : uint8_t {
    FADD,
    FSUB,
    FMUL,
    FVID,
    FREM,  /// floating point division remainder
};

enum class BinaryBitwiseOperator : uint8_t {
    SHL,   /// shift left
    LSHR,  /// logical shift right (zero fill)
    ASHR,  /// arithmetic shift right (with sign extension)
    AND,
    OR,
    XOR
};

enum class IntegerComparisonOperator : uint8_t {
    EQ,   /// equal
    NE,   /// not equal
    UGT,  /// unsigned comparisons
    UGE,
    ULT,
    ULE,
    SGT,  /// signed comparison
    SGE,
    SLT,
    SLE,
};

// clang-format off
enum class FloatingPointComparisonOperator : uint8_t {
    FALSE,  /// always false
    OEQ,    /// ordered and ...
    OGT,
    OGE,
    OLT,
    OLE,
    ONE,
    ORD,    /// ordered (neither are nans)
    UEQ,    /// unordered or ...
    UGT,
    UGE,
    ULT,
    ULE,
    UNE,
    UNO,    /// unordered (either is nan)
    TRUE    /// always true
};
// clang-format on

}  // namespace lir

std::string to_string(lir::BinaryIntegerOperator);
std::string to_string(lir::BinaryFloatingPointOperator);
std::string to_string(lir::BinaryBitwiseOperator);
std::string to_string(lir::IntegerComparisonOperator);
std::string to_string(lir::FloatingPointComparisonOperator);

template <>
struct fmt::formatter<lir::BinaryIntegerOperator> : formatter<string_view> {
    auto format(const lir::BinaryIntegerOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<lir::BinaryFloatingPointOperator> : formatter<string_view> {
    auto format(const lir::BinaryFloatingPointOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<lir::BinaryBitwiseOperator> : formatter<string_view> {
    auto format(const lir::BinaryBitwiseOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<lir::IntegerComparisonOperator> : formatter<string_view> {
    auto format(const lir::IntegerComparisonOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<lir::FloatingPointComparisonOperator> : formatter<string_view> {
    auto format(
        const lir::FloatingPointComparisonOperator op, format_context& ctx
    ) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};
