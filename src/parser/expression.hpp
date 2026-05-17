#pragma once

#include <memory>
#include <vector>

#include "utils/atom.hpp"
#include "utils/operator.hpp"

namespace ast {

class ExpressionVisitor;

struct Expression {
    virtual ~Expression() = default;
    virtual void accept(ExpressionVisitor* v) = 0;
};

struct PrimaryExpression : Expression {};

struct AtomExpression final : PrimaryExpression {
    std::unique_ptr<Atom> atom;

    AtomExpression();
    explicit AtomExpression(std::unique_ptr<Atom> atom);
    void accept(ExpressionVisitor* v) override;
};

struct ParenExpression final : PrimaryExpression {
    std::unique_ptr<Expression> expression;

    ParenExpression();
    explicit ParenExpression(std::unique_ptr<Expression> expression);
    void accept(ExpressionVisitor* v) override;
};

struct BinaryExpression final : Expression {
    BinaryOperator op{};
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpression();
    BinaryExpression(
        BinaryOperator op, std::unique_ptr<Expression> left,
        std::unique_ptr<Expression> right
    );
    void accept(ExpressionVisitor* v) override;
};

struct UnaryExpression final : PrimaryExpression {
    UnaryOperator op{};
    std::unique_ptr<PrimaryExpression> expression;

    UnaryExpression();
    UnaryExpression(UnaryOperator op, std::unique_ptr<PrimaryExpression> expression);
    void accept(ExpressionVisitor* v) override;
};

struct CallExpression final : PrimaryExpression {
    std::unique_ptr<AtomIdentifier> function;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression();
    CallExpression(
        std::unique_ptr<AtomIdentifier> function,
        std::vector<std::unique_ptr<Expression>> arguments
    );
    void accept(ExpressionVisitor* v) override;
};

class ExpressionVisitor {
public:
    virtual void visit(AtomExpression*) = 0;
    virtual void visit(ParenExpression*) = 0;
    virtual void visit(BinaryExpression*) = 0;
    virtual void visit(UnaryExpression*) = 0;
    virtual void visit(CallExpression*) = 0;

    virtual ~ExpressionVisitor() = default;
};
}  // namespace ast
