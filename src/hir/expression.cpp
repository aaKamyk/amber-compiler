#include "expression.hpp"

hir::TerminalExpression::TerminalExpression(std::unique_ptr<Atom> value)
    : value(std::move(value)) {}

void hir::TerminalExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::BinaryExpression::BinaryExpression(
    const BinaryOperator op, std::unique_ptr<Atom> left, std::unique_ptr<Atom> right
)
    : op(op), left(std::move(left)), right(std::move(right)) {}

void hir::BinaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::UnaryExpression::UnaryExpression(
    const UnaryOperator op, std::unique_ptr<Atom> operand
)
    : op(op), operand(std::move(operand)) {}

void hir::UnaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::ComparisonExpression::ComparisonExpression(
    const ComparisonOperator op, std::unique_ptr<Atom> left, std::unique_ptr<Atom> right
)
    : op(op), left(std::move(left)), right(std::move(right)) {}

void hir::ComparisonExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}
