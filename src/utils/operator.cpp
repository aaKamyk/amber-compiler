#include "operator.hpp"

#include <map>
#include <stdexcept>

int get_precedence(const ast::BinaryOperator op) {
    static const std::map<ast::BinaryOperator, int> precedences = {
        {ast::BinaryOperator::EQUAL, 3},   {ast::BinaryOperator::NOT_EQUAL, 3},
        {ast::BinaryOperator::GREATER, 4}, {ast::BinaryOperator::GREATER_EQUAL, 4},
        {ast::BinaryOperator::LESS, 4},    {ast::BinaryOperator::LESS_EQUAL, 4},
        {ast::BinaryOperator::PLUS, 5},    {ast::BinaryOperator::MINUS, 5},
        {ast::BinaryOperator::TIMES, 6},   {ast::BinaryOperator::DIVIDE, 6},
    };
    return precedences.at(op);
}

int get_precedence(const ast::UnaryOperator op) {
    static const std::map<ast::UnaryOperator, int> precedences = {
        {ast::UnaryOperator::MINUS, 1}
    };
    return precedences.at(op);
}

std::string to_string(const ast::BinaryOperator op) {
    static const std::map<ast::BinaryOperator, const char*> bin_op_strings = {
        {ast::BinaryOperator::PLUS, "+"},    {ast::BinaryOperator::MINUS, "-"},
        {ast::BinaryOperator::TIMES, "*"},   {ast::BinaryOperator::DIVIDE, "/"},
        {ast::BinaryOperator::EQUAL, "=="},  {ast::BinaryOperator::NOT_EQUAL, "!="},
        {ast::BinaryOperator::LESS, "<"},    {ast::BinaryOperator::LESS_EQUAL, "<="},
        {ast::BinaryOperator::GREATER, ">"}, {ast::BinaryOperator::GREATER_EQUAL, ">="}
    };
    if (!bin_op_strings.contains(op)) {
        throw std::runtime_error("Unknown binary operator");
    }
    return bin_op_strings.at(op);
}

std::string to_string(const ast::UnaryOperator op) {
    static const std::map<ast::UnaryOperator, const char*> un_op_strings = {
        {ast::UnaryOperator::MINUS, "-"}
    };
    if (!un_op_strings.contains(op)) {
        throw std::runtime_error("Unknown unary operator");
    }
    return un_op_strings.at(op);
}

std::string to_string(const hir::BinaryOperator op) {
    static const std::map<hir::BinaryOperator, const char*> bin_op_strings = {
        {hir::BinaryOperator::PLUS, "+"},
        {hir::BinaryOperator::MINUS, "-"},
        {hir::BinaryOperator::TIMES, "*"},
        {hir::BinaryOperator::DIVIDE, "/"}
    };
    if (!bin_op_strings.contains(op)) {
        throw std::runtime_error("Unknown binary operator");
    }
    return bin_op_strings.at(op);
}

std::string to_string(const hir::UnaryOperator op) {
    static const std::map<hir::UnaryOperator, const char*> un_op_strings = {
        {hir::UnaryOperator::MINUS, "-"},
    };
    if (!un_op_strings.contains(op)) {
        throw std::runtime_error("Unknown unary operator");
    }
    return un_op_strings.at(op);
}

std::string to_string(const hir::ComparisonOperator op) {
    static const std::map<hir::ComparisonOperator, const char*> comp_op_strings = {
        {hir::ComparisonOperator::EQUAL, "=="},
        {hir::ComparisonOperator::NOT_EQUAL, "!="},
        {hir::ComparisonOperator::GREATER, ">"},
        {hir::ComparisonOperator::GREATER_EQUAL, ">="},
        {hir::ComparisonOperator::LESS, "<"},
        {hir::ComparisonOperator::LESS_EQUAL, "<="},
    };
    if (!comp_op_strings.contains(op)) {
        throw std::runtime_error("Unknown comparison operator");
    }
    return comp_op_strings.at(op);
}
