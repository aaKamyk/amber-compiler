#pragma once

#include <memory>
#include <variant>

#include "utils/atom.hpp"
#include "utils/operator.hpp"

namespace hir {

struct BinaryExpression;
struct UnaryExpression;

struct TerminalExpression {
    std::unique_ptr<Atom> value;
};

struct Expression {
    using Expr = std::variant<
        std::unique_ptr<BinaryExpression>, std::unique_ptr<UnaryExpression>,
        std::unique_ptr<TerminalExpression>>;
    Expr expression;
};

struct BinaryExpression {
    BinaryOperator op;
    std::unique_ptr<Atom> left;
    std::unique_ptr<Atom> right;
};

struct UnaryExpression {
    UnaryOperator op;
    std::unique_ptr<Atom> operand;
};

struct ComparisonExpression {
    ComparisonOperator op;
    std::unique_ptr<Atom> left;
    std::unique_ptr<Atom> right;
};

}  // namespace hir