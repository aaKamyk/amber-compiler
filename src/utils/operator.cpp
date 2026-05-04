#include "operator.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>

std::ostream& operator<<(std::ostream& os, const BinaryOperator op) {
    static const std::unordered_map<BinaryOperator, const char*> bin_op_strings = {
        {BinaryOperator::PLUS, "+"},
        {BinaryOperator::MINUS, "-"},
        {BinaryOperator::TIMES, "*"},
        {BinaryOperator::DIVIDE, "/"}
    };
    if (!bin_op_strings.contains(op)) { throw std::runtime_error("Unknown operator"); }
    return os << bin_op_strings.at(op);
}

std::ostream& operator<<(std::ostream& os, const UnaryOperator op) {
    static const std::unordered_map<UnaryOperator, const char*> un_op_strings = {
        {UnaryOperator::MINUS, "-"}
    };
    if (!un_op_strings.contains(op)) { throw std::runtime_error("Unknown operator"); }
    return os << un_op_strings.at(op);
}

int get_precedence(const BinaryOperator op) {
    static const std::unordered_map<BinaryOperator, int> precedences = {
        {BinaryOperator::PLUS, 1},
        {BinaryOperator::MINUS, 1},
        {BinaryOperator::TIMES, 2},
        {BinaryOperator::DIVIDE, 2},
    };
    return precedences.at(op);
}

int get_precedence(const UnaryOperator op) {
    static const std::unordered_map<UnaryOperator, int> precedences = {
        {UnaryOperator::MINUS, 1}
    };
    return precedences.at(op);
}