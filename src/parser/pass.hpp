#pragma once

#include "node.hpp"

namespace ast {

struct Pass : NodeVisitor, ExpressionVisitor {
    virtual Program run() = 0;
    ~Pass() override = default;
};

class ConstantFold final : Pass {
public:
    explicit ConstantFold(Program program)
        : m_program(std::move(program)) {};

    Program run() override;

private:
    Program m_program;
    std::unique_ptr<Expression> m_current_expression{nullptr};

    template <typename T>
        requires std::is_base_of_v<Node, T>
    void pass(std::unique_ptr<T>& node) {
        node->accept(this);
    }
    template <typename T>
        requires std::is_base_of_v<Expression, T>
    void pass(std::unique_ptr<T>& expr) {
        expr->accept(this);
        if (m_current_expression) { expr = std::move(m_current_expression); }
    }

    void visit(BlockStatement* block_statement) override;
    void visit(AssignmentStatement* assignment_statement) override;
    void visit(ReturnStatement* return_statement) override;
    void visit(IfStatement* if_statement) override;
    void visit(WhileStatement* while_statement) override;
    void visit(ExpressionStatement* expression_statement) override;

    void visit(VariableDeclaration* variable_declaration) override;
    void visit(FunctionArgument* function_argument) override;
    void visit(FunctionDeclaration* function_declaration) override;

    void visit(AtomExpression* atom_expression) override;
    void visit(ParenExpression* paren_expression) override;
    void visit(BinaryExpression* binary_expression) override;
    void visit(UnaryExpression* unary_expression) override;
    void visit(CallExpression* call_expression) override;
};
}  // namespace ast