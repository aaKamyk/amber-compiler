#pragma once

#include <cstdint>
#include <iosfwd>

enum class BinaryOperator : std::uint8_t { PLUS, MINUS, TIMES, DIVIDE };
enum class UnaryOperator : std::uint8_t { MINUS };

std::ostream& operator<<(std::ostream& os, BinaryOperator op);
std::ostream& operator<<(std::ostream& os, UnaryOperator op);
