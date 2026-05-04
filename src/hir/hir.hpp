#pragma once

#include <map>
#include <optional>
#include <unordered_set>

#include "instruction.hpp"
#include "parser/node.hpp"
#include "utils/symbolTable.hpp"

namespace hir {

struct Program {
    std::vector<InstructionDeclaration> declarations;
};

struct Scope {
    uint64_t depth;
    std::unordered_set<uint64_t> symbols;

    bool contains(uint64_t symbol) const;
    void add(uint64_t symbol);
};

class Hir final : ast::NodeVisitor, ast::ExpressionVisitor {
public:
    explicit Hir(ast::Program program);

    Program translate();

private:
    template <typename Node>
    void translate(std::unique_ptr<Node> node) {
        node->accept(this);
    }

    void visit(ast::BlockStatement* block_statement) override;
    void visit(ast::AssignmentStatement* assignment_statement) override;
    void visit(ast::ReturnStatement* return_statement) override;
    void visit(ast::IfStatement* if_statement) override;
    void visit(ast::WhileStatement* while_statement) override;

    void visit(ast::VariableDeclaration* variable_declaration) override;
    void visit(ast::FunctionArgument* function_argument) override;
    void visit(ast::FunctionDeclaration* function_declaration) override;

    void visit(ast::AtomExpression* atom_expression) override;
    void visit(ast::ParenExpression* paren_expression) override;
    void visit(ast::BinaryExpression* binary_expression) override;
    void visit(ast::UnaryExpression* unary_expression) override;

    void translate(std::unique_ptr<ast::IfStatement> stmt);
    void translate(std::unique_ptr<ast::WhileStatement> stmt);
    void translate(std::unique_ptr<ast::ReturnStatement> stmt);

    std::unique_ptr<AtomIdentifier> create_temporary(const std::string& name);
    std::optional<uint64_t> resolve_scope(const AtomIdentifier* atom) const;
    const Scope& current_scope() const;
    Scope& current_scope();
    void push_scope();
    void pop_scope();

    void set_active_function(std::vector<Instruction>&);
    void unset_active_function();
    void append_to_active_func(Instruction) const;
    void append_to_program(InstructionDeclaration);

    std::unique_ptr<Expression> get_last_expression();

    std::vector<Scope> m_active_scopes;
    std::vector<Instruction>* m_active_function;
    std::vector<std::unique_ptr<Expression>> m_active_expressions;
    std::map<std::string, uint64_t> m_temporary_counts;
    ast::Program m_ast_program;
    Program m_program;
    SymbolTable m_symbol_table{};
};

void print_program(const Program&);

}  // namespace hir