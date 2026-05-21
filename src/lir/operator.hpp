#pragma once

#include <cstdint>
#include <iosfwd>

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

std::ostream& operator<<(std::ostream& os, const lir::BinaryIntegerOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::BinaryFloatingPointOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::BinaryBitwiseOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::IntegerComparisonOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::FloatingPointComparisonOperator& op);
