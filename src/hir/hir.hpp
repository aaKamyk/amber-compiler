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

class Hir final : ast::NodeVisitor<type::TypeID>, ast::ExpressionVisitor {
public:
    explicit Hir(ast::Program<type::TypeID> program, CompilationContext& ctx);

    Program translate();
    bool was_successful() const;

    void visit(ast::BlockStatement<type::TypeID>* block_statement) override;
    void visit(ast::AssignmentStatement<type::TypeID>* assignment_statement) override;
    void visit(ast::ReturnStatement<type::TypeID>* return_statement) override;
    void visit(ast::IfStatement<type::TypeID>* if_statement) override;
    void visit(ast::WhileStatement<type::TypeID>* while_statement) override;
    void visit(ast::ExpressionStatement<type::TypeID>* expression_statement) override;

    void visit(ast::VariableDeclaration<type::TypeID>* variable_declaration) override;
    void visit(ast::FunctionParameter<type::TypeID>* function_argument) override;
    void visit(ast::FunctionDeclaration<type::TypeID>* function_declaration) override;
    void visit(ast::StructField<type::TypeID>* struct_field) override;
    void visit(ast::StructDeclaration<type::TypeID>* struct_declaration) override;

    void visit(ast::AtomExpression* atom_expression) override;
    void visit(ast::ParenExpression* paren_expression) override;
    void visit(ast::BinaryExpression* binary_expression) override;
    void visit(ast::UnaryExpression* unary_expression) override;
    void visit(ast::CallExpression* call_expression) override;

private:
    template <typename Node>
    void translate(std::unique_ptr<Node> node) {
        node->accept(this);
    }

    std::unique_ptr<AtomIdentifier> create_temporary_identifier(const std::string& name);
    std::unique_ptr<AtomIdentifier> create_temporary_declaration(
        const std::string& name, type::TypeID type
    );
    std::unique_ptr<AtomIdentifier> create_temporary_label(const std::string& name);
    std::optional<std::string> resolve_scope(const AtomIdentifier& atom) const;
    type::TypeID resolve_type(const Atom& atom) const;
    type::TypeID resolve_type(const std::string& ident) const;
    const Scope& current_scope() const;
    Scope& current_scope();
    void push_scope();
    void pop_scope();

    void set_active_function(std::vector<std::unique_ptr<Instruction>>&);
    void unset_active_function();
    void append_to_active_func(std::unique_ptr<Instruction>);
    void append_to_program(std::unique_ptr<InstructionFunctionDeclaration>);
    void append_global_init_to_program();
    void add_error(const ast::Node<type::TypeID>*, const std::string& msg);

    std::unique_ptr<Expression> get_last_expression();
    const Expression* view_last_expression() const;
    std::unique_ptr<Atom> ensure_atom(std::unique_ptr<Expression> expr);
    Instruction& get_last_instruction() const;

    std::vector<Scope> m_active_scopes;
    std::vector<std::unique_ptr<Instruction>>* m_active_function;
    std::vector<std::unique_ptr<Instruction>> m_global_init;
    std::vector<std::unique_ptr<Expression>> m_active_expressions;
    std::map<std::string, uint64_t> m_temporary_counts;
    ast::Program<type::TypeID> m_ast_program{};
    Program m_program;
    SymbolTable m_symbol_table{};
    CompilationContext& m_ctx;
    bool m_translated_successfully{true};
};

Program translate(ast::Program<type::TypeID>, CompilationContext& ctx);
void print_program(const Program&, std::ostream& out);

}  // namespace hir