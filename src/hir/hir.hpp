#pragma once

#include <optional>
#include <unordered_set>

#include "instruction.hpp"
#include "parser/ast.hpp"
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

class Hir {
public:
    explicit Hir(ASTProgram program);

    Program translate();

private:
    void translate(std::unique_ptr<ASTExpression> expr);
    void translate(std::unique_ptr<ASTPrimaryExpression> expr);
    void translate(std::unique_ptr<ASTIdentifier> ident);
    void translate(std::unique_ptr<ASTIntegerLiteral> lit);
    void translate(std::unique_ptr<ASTParenExpression> expr);
    void translate(std::unique_ptr<ASTUnaryExpression> expr);
    void translate(std::unique_ptr<ASTBinaryExpression> expr);
    void translate(std::unique_ptr<ASTDeclaration> decl);
    void translate(std::unique_ptr<ASTVariableDeclaration> decl);
    void translate(std::unique_ptr<ASTFunctionDeclaration> decl);
    void translate(std::unique_ptr<ASTStatement> stmt);
    void translate(std::unique_ptr<ASTBlockStatement> stmt);
    void translate(std::unique_ptr<ASTAssignmentStatement> stmt);
    void translate(std::unique_ptr<ASTIfStatement> stmt);
    void translate(std::unique_ptr<ASTWhileStatement> stmt);
    void translate(std::unique_ptr<ASTReturnStatement> stmt);

    std::optional<uint64_t> resolve_scope(const AtomIdentifier* atom) const;
    const Scope& current_scope() const;
    Scope& current_scope();
    void push_scope();
    void pop_scope();

    void set_active_function(std::vector<Instruction>&);
    void unset_active_function();
    void append_to_program(InstructionDeclaration);

    ASTProgram m_ast_program;
    Program m_program;
    std::vector<Scope> m_active_scopes;
    std::vector<Instruction>* m_active_function;
    std::vector<Expression> m_active_expressions;
    SymbolTable m_symbol_table{};
};

}  // namespace hir