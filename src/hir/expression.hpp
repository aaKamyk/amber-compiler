#pragma once

#include <memory>
#include <variant>

namespace Hir {

struct BinaryExpression;
struct UnaryExpression;

struct Expression {
    using Expr = std::variant<std::unique_ptr<BinaryExpression>, std::unique_ptr<UnaryExpression>>;
    Expr expression;
};

struct BinaryExpression {
    Expression right;
    Expression left;
};
}  // namespace Hir