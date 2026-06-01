#pragma once

#include <map>
#include <optional>

#include "instruction.hpp"
#include "parser/node.hpp"
#include "utils/context.hpp"
#include "utils/symbolTable.hpp"

namespace hir {

struct Program {
    std::vector<std::unique_ptr<InstructionFunctionDeclaration>> functions;
};

struct Scope {
    uint64_t depth;
    std::unordered_map<std::string, std::string> symbols;

    bool contains(std::string_view name) const;
    void add(std::string name, std::string unique_name);
    std::string get_unique_name(std::string_view name) const;
};

class Hir final : ast::NodeVisitor, ast::ExpressionVisitor {
public:
    explicit Hir(ast::Program program, CompilationContext& ctx);

    Program translate();
    bool was_successful() const;

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
    void visit(ast::ExpressionStatement* expression_statement) override;

    void visit(ast::VariableDeclaration* variable_declaration) override;
    void visit(ast::FunctionParameter* function_argument) override;
    void visit(ast::FunctionDeclaration* function_declaration) override;

    void visit(ast::AtomExpression* atom_expression) override;
    void visit(ast::ParenExpression* paren_expression) override;
    void visit(ast::BinaryExpression* binary_expression) override;
    void visit(ast::UnaryExpression* unary_expression) override;
    void visit(ast::CallExpression* call_expression) override;

    std::unique_ptr<AtomIdentifier> create_temporary_identifier(const std::string& name);
    std::unique_ptr<AtomIdentifier> create_temporary_declaration(
        const std::string& name, TypeID type
    );
    std::unique_ptr<AtomIdentifier> create_temporary_label(const std::string& name);
    std::optional<std::string> resolve_scope(const AtomIdentifier& atom) const;
    TypeID resolve_type(const Atom& atom) const;
    TypeID resolve_type(const std::string& ident) const;
    const Scope& current_scope() const;
    Scope& current_scope();
    void push_scope();
    void pop_scope();

    void set_active_function(std::vector<std::unique_ptr<Instruction>>&);
    void unset_active_function();
    void append_to_active_func(std::unique_ptr<Instruction>);
    void append_to_program(std::unique_ptr<InstructionFunctionDeclaration>);
    void append_global_init_to_program();
    void add_error(const ast::Node*, const std::string& msg);

    std::unique_ptr<Expression> get_last_expression();
    const Expression* view_last_expression() const;
    std::unique_ptr<Atom> ensure_atom(std::unique_ptr<Expression> expr);
    Instruction& get_last_instruction() const;

    std::vector<Scope> m_active_scopes;
    std::vector<std::unique_ptr<Instruction>>* m_active_function;
    std::vector<std::unique_ptr<Instruction>> m_global_init;
    std::vector<std::unique_ptr<Expression>> m_active_expressions;
    std::map<std::string, uint64_t> m_temporary_counts;
    ast::Program m_ast_program;
    Program m_program;
    SymbolTable m_symbol_table{};
    CompilationContext& m_ctx;
    bool m_translated_successfully{true};
};

Program translate(ast::Program, CompilationContext& ctx);
void print_program(const Program&);

}  // namespace hir