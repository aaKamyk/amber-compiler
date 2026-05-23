#pragma once

#include <cstdint>

#include "fmt/core.h"

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
enum class UnaryOperator : std::uint8_t { MINUS, NOT };
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

std::string to_string(ast::BinaryOperator op);
std::string to_string(ast::UnaryOperator op);
std::string to_string(hir::BinaryOperator op);
std::string to_string(hir::UnaryOperator op);
std::string to_string(hir::ComparisonOperator op);

template <>
struct fmt::formatter<ast::BinaryOperator> : formatter<string_view> {
    auto format(const ast::BinaryOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<ast::UnaryOperator> : formatter<string_view> {
    auto format(const ast::UnaryOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<hir::BinaryOperator> : formatter<string_view> {
    auto format(const hir::BinaryOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<hir::UnaryOperator> : formatter<string_view> {
    auto format(const hir::UnaryOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<hir::ComparisonOperator> : formatter<string_view> {
    auto format(const hir::ComparisonOperator op, format_context& ctx) const {
        const string_view name = ::to_string(op);
        return formatter<string_view>::format(name, ctx);
    }
};
