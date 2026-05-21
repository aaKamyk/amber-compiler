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

enum class BinaryFloatingOperator : uint8_t {
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

}  // namespace lir

std::ostream& operator<<(std::ostream& os, const lir::BinaryIntegerOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::BinaryFloatingOperator& op);
std::ostream& operator<<(std::ostream& os, const lir::BinaryBitwiseOperator& op);
