#pragma once

#include <memory>
#include <vector>

#include "utils/atom.hpp"
#include "utils/operator.hpp"
#include "utils/type.hpp"

namespace hir {

class ExpressionVisitor;

struct Expression {
    Type result_type{Type::VOID};

    explicit Expression(const Type result_type)
        : result_type(result_type) {}
    explicit Expression() = default;
    virtual void accept(ExpressionVisitor* v) = 0;
    virtual ~Expression() = default;
};

struct TerminalExpression final : Expression {
    std::unique_ptr<Atom> value;

    explicit TerminalExpression(Type type, std::unique_ptr<Atom> value);
    void accept(ExpressionVisitor* v) override;
};

struct BinaryExpression final : Expression {
    BinaryOperator op;
    std::unique_ptr<Atom> left;
    std::unique_ptr<Atom> right;

    BinaryExpression(
        Type type, BinaryOperator op, std::unique_ptr<Atom> left,
        std::unique_ptr<Atom> right
    );
    void accept(ExpressionVisitor* v) override;
};

struct UnaryExpression final : Expression {
    UnaryOperator op;
    std::unique_ptr<Atom> operand;

    UnaryExpression(Type type, UnaryOperator op, std::unique_ptr<Atom> operand);
    void accept(ExpressionVisitor* v) override;
};

struct ComparisonExpression final : Expression {
    ComparisonOperator op;
    std::unique_ptr<Atom> left;
    std::unique_ptr<Atom> right;

    ComparisonExpression(
        Type type, ComparisonOperator op, std::unique_ptr<Atom> left,
        std::unique_ptr<Atom> right
    );
    void accept(ExpressionVisitor* v) override;
};

struct CallExpression final : Expression {
    std::vector<std::unique_ptr<Atom>> arguments{};
    std::unique_ptr<AtomIdentifier> callee{nullptr};

    CallExpression() = default;
    CallExpression(
        Type type, std::vector<std::unique_ptr<Atom>> arguments,
        std::unique_ptr<AtomIdentifier> callee
    );
    void accept(ExpressionVisitor* v) override;
};

class ExpressionVisitor {
public:
    virtual void visit(BinaryExpression*) = 0;
    virtual void visit(UnaryExpression*) = 0;
    virtual void visit(ComparisonExpression*) = 0;
    virtual void visit(TerminalExpression*) = 0;
    virtual void visit(CallExpression*) = 0;

    virtual ~ExpressionVisitor() = default;
};

}  // namespace hir