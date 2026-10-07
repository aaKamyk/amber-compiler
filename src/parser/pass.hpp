#pragma once

#include "node.hpp"

namespace ast {

template <TypeMetadata InputInfo, TypeMetadata ResultInfo = InputInfo>
struct Pass : NodeVisitor<InputInfo>, ExpressionVisitor {
    virtual Program<ResultInfo> run() = 0;
    ~Pass() override = default;
};

class ConstantFold final : Pass<type::TypeID> {
public:
    explicit ConstantFold(Program<type::TypeID> program)
        : m_program(std::move(program)) {};

    Program<type::TypeID> run() override;

    void visit(BlockStatement<type::TypeID>* block_statement) override;
    void visit(AssignmentStatement<type::TypeID>* assignment_statement) override;
    void visit(ReturnStatement<type::TypeID>* return_statement) override;
    void visit(IfStatement<type::TypeID>* if_statement) override;
    void visit(WhileStatement<type::TypeID>* while_statement) override;
    void visit(ExpressionStatement<type::TypeID>* expression_statement) override;

    void visit(VariableDeclaration<type::TypeID>* variable_declaration) override;
    void visit(FunctionParameter<type::TypeID>* function_argument) override;
    void visit(FunctionDeclaration<type::TypeID>* function_declaration) override;
    void visit(StructField<type::TypeID>* struct_field) override;
    void visit(StructDeclaration<type::TypeID>* struct_declaration) override;

    void visit(AtomExpression* atom_expression) override;
    void visit(ParenExpression* paren_expression) override;
    void visit(BinaryExpression* binary_expression) override;
    void visit(UnaryExpression* unary_expression) override;
    void visit(CallExpression* call_expression) override;

private:
    Program<type::TypeID> m_program;
    std::unique_ptr<Expression> m_current_expression{nullptr};

    template <typename T>
        requires std::is_base_of_v<Node<type::TypeID>, T>
    void pass(std::unique_ptr<T>& node) {
        node->accept(this);
    }
    template <typename T>
        requires std::is_base_of_v<Expression, T>
    void pass(std::unique_ptr<T>& expr) {
        expr->accept(this);
        if (m_current_expression) { expr = std::move(m_current_expression); }
    }
};

class CollectTypes final : Pass<type::ast::Type> {
public:
    Program<type::ast::Type> run() override;

    explicit CollectTypes(Program<type::ast::Type> program)
        : m_program(std::move(program)) {};

    void visit(BlockStatement<type::ast::Type>* block_statement) override;
    void visit(AssignmentStatement<type::ast::Type>* assignment_statement) override {}
    void visit(ReturnStatement<type::ast::Type>* return_statement) override {}
    void visit(IfStatement<type::ast::Type>* if_statement) override {}
    void visit(WhileStatement<type::ast::Type>* while_statement) override {}
    void visit(ExpressionStatement<type::ast::Type>* expression_statement) override {}

    void visit(VariableDeclaration<type::ast::Type>* variable_declaration) override {}
    void visit(FunctionParameter<type::ast::Type>* function_argument) override {}
    void visit(FunctionDeclaration<type::ast::Type>* function_declaration) override {}
    void visit(StructField<type::ast::Type>* struct_field) override {}
    void visit(StructDeclaration<type::ast::Type>* struct_declaration) override;

    void visit(AtomExpression* atom_expression) override {}
    void visit(ParenExpression* paren_expression) override {}
    void visit(BinaryExpression* binary_expression) override {}
    void visit(UnaryExpression* unary_expression) override {}
    void visit(CallExpression* call_expression) override {}

private:
    Program<type::ast::Type> m_program;

    template <typename T>
    void pass(std::unique_ptr<T>& node) {
        node->accept(this);
    }
};

class ResolveTypes final : Pass<type::ast::Type, type::TypeID> {
public:
    explicit ResolveTypes(Program<type::ast::Type> program)
        : m_program(std::move(program)) {
        const auto program_size = this->m_program.children.size();
        m_resolved_program.children.reserve(program_size);
    }

    Program<type::TypeID> run() override;

    void visit(BlockStatement<type::ast::Type>* block_statement) override;
    void visit(AssignmentStatement<type::ast::Type>* assignment_statement) override;
    void visit(ReturnStatement<type::ast::Type>* return_statement) override;
    void visit(IfStatement<type::ast::Type>* if_statement) override;
    void visit(WhileStatement<type::ast::Type>* while_statement) override;
    void visit(ExpressionStatement<type::ast::Type>* expression_statement) override;

    void visit(VariableDeclaration<type::ast::Type>* variable_declaration) override;
    void visit(FunctionParameter<type::ast::Type>* function_argument) override;
    void visit(FunctionDeclaration<type::ast::Type>* function_declaration) override;
    void visit(StructField<type::ast::Type>* struct_field) override;
    void visit(StructDeclaration<type::ast::Type>* struct_declaration) override;

    void visit(AtomExpression* atom_expression) override;
    void visit(ParenExpression* paren_expression) override;
    void visit(BinaryExpression* binary_expression) override;
    void visit(UnaryExpression* unary_expression) override;
    void visit(CallExpression* call_expression) override;

private:
    Program<type::ast::Type> m_program;
    Program<type::TypeID> m_resolved_program;

    // Transformation stack
    std::vector<std::unique_ptr<ast::Node<type::TypeID>>> m_result_stack;

    template <typename TargetNode, typename SourceNode>
    std::unique_ptr<TargetNode> transform(SourceNode* node) {
        if (node == nullptr) { return nullptr; }

        node->accept(this);
        auto result = std::move(m_result_stack.back());
        m_result_stack.pop_back();

        return std::unique_ptr<TargetNode>{dynamic_cast<TargetNode*>(result.release())};
    }

    template <typename T>
    void pass(std::unique_ptr<T>& node) {
        node->accept(this);
    }

    static type::TypeID resolve_type(const type::ast::Type& type);
};

}  // namespace ast