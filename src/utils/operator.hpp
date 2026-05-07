#pragma once

#include <cstdint>
#include <iosfwd>

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

int get_precedence(BinaryOperator op);
int get_precedence(UnaryOperator op);

std::ostream& operator<<(std::ostream& os, BinaryOperator op);
std::ostream& operator<<(std::ostream& os, UnaryOperator op);
std::ostream& operator<<(std::ostream& os, ComparisonOperator op);
