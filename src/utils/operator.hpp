#pragma once

#include <cstdint>
#include <iosfwd>

namespace ast {
enum class BinaryOperator : std::uint8_t {
    /// arithmetic operators
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    /// logical operators
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL
};
enum class UnaryOperator : std::uint8_t { MINUS };
}  // namespace ast

namespace hir {
enum class BinaryOperator : std::uint8_t { PLUS, MINUS, TIMES, DIVIDE };
enum class UnaryOperator : std::uint8_t { MINUS };
enum class ComparisonOperator : std::uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL
};

}  // namespace hir

int get_precedence(ast::BinaryOperator op);
int get_precedence(ast::UnaryOperator op);

std::ostream& operator<<(std::ostream& os, ast::BinaryOperator op);
std::ostream& operator<<(std::ostream& os, ast::UnaryOperator op);
std::ostream& operator<<(std::ostream& os, hir::BinaryOperator op);
std::ostream& operator<<(std::ostream& os, hir::UnaryOperator op);
std::ostream& operator<<(std::ostream& os, hir::ComparisonOperator op);
