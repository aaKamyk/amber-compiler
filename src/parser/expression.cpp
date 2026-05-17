#include "expression.hpp"

ast::AtomExpression::AtomExpression()
    : atom(nullptr) {}

ast::AtomExpression::AtomExpression(std::unique_ptr<Atom> atom)
    : atom(std::move(atom)) {}

void ast::AtomExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

ast::ParenExpression::ParenExpression()
    : expression(nullptr) {}

ast::ParenExpression::ParenExpression(std::unique_ptr<Expression> expression)
    : expression(std::move(expression)) {}

void ast::ParenExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

ast::BinaryExpression::BinaryExpression()
    : left(nullptr), right(nullptr) {}

ast::BinaryExpression::BinaryExpression(
    const BinaryOperator op, std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right
)
    : op(op), left(std::move(left)), right(std::move(right)) {}

void ast::BinaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

ast::UnaryExpression::UnaryExpression()
    : expression(nullptr) {}

ast::UnaryExpression::UnaryExpression(
    const UnaryOperator op, std::unique_ptr<PrimaryExpression> expression
)
    : op(op), expression(std::move(expression)) {}

void ast::UnaryExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}

ast::CallExpression::CallExpression()
    : function(nullptr) {}

ast::CallExpression::CallExpression(
    std::unique_ptr<AtomIdentifier> function,
    std::vector<std::unique_ptr<Expression>> arguments
)
    : function(std::move(function)), arguments(std::move(arguments)) {}

void ast::CallExpression::accept(ExpressionVisitor* v) {
    v->visit(this);
}