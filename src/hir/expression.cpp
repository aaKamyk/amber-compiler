#include "expression.hpp"

hir::TerminalExpression::TerminalExpression(const Type type, std::unique_ptr<Atom> value)
    : Expression(type), value(std::move(value)) {}

void hir::TerminalExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::BinaryExpression::BinaryExpression(
    const Type type, const BinaryOperator op, std::unique_ptr<Atom> left,
    std::unique_ptr<Atom> right
)
    : Expression(type), op(op), left(std::move(left)), right(std::move(right)) {}

void hir::BinaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::UnaryExpression::UnaryExpression(
    const Type type, const UnaryOperator op, std::unique_ptr<Atom> operand
)
    : Expression(type), op(op), operand(std::move(operand)) {}

void hir::UnaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

hir::ComparisonExpression::ComparisonExpression(
    const Type type, const ComparisonOperator op, std::unique_ptr<Atom> left,
    std::unique_ptr<Atom> right
)
    : Expression(type), op(op), left(std::move(left)), right(std::move(right)) {}

void hir::ComparisonExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}
hir::CallExpression::CallExpression(
    const Type type, std::vector<std::unique_ptr<Atom>> arguments,
    std::unique_ptr<AtomIdentifier> callee
)
    : Expression(type), arguments(std::move(arguments)), callee(std::move(callee)) {}

void hir::CallExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}
