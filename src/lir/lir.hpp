#pragma once

#include "function.hpp"
#include "hir/expression.hpp"
#include "hir/hir.hpp"
#include "hir/instruction.hpp"
#include "utils/context.hpp"

namespace lir {

struct Program {
    std::vector<FunctionDeclaration> function_declarations;
    std::vector<FunctionDefinition> functions;
};

class Lir final : hir::InstructionVisitor, hir::ExpressionVisitor {
public:
    explicit Lir(hir::Program program);
    Program translate();

    void visit(hir::Label*) override;

    void visit(hir::InstructionDiscard*) override;
    void visit(hir::InstructionAssignment*) override;
    void visit(hir::InstructionReturn*) override;
    void visit(hir::InstructionBranch*) override;
    void visit(hir::InstructionBranchConditional*) override;
    void visit(hir::InstructionVariableDeclaration*) override;
    void visit(hir::InstructionFunctionDeclaration*) override;

    void visit(hir::BinaryExpression*) override;
    void visit(hir::UnaryExpression*) override;
    void visit(hir::ComparisonExpression*) override;
    void visit(hir::TerminalExpression*) override;
    void visit(hir::CallExpression*) override;

    void set_active_func(FunctionDefinition*);
    [[nodiscard]] FunctionDefinition* get_active_func() const;
    std::unique_ptr<Atom> get_last_value();
    std::unique_ptr<Atom> ensure_value(std::unique_ptr<Atom> at);

    static std::unique_ptr<AtomIdentifier> get_temporary_identifier(
        const std::string& name
    );

    hir::Program m_program;
    Program m_lir_program;
    FunctionDefinition* m_active_func;
    std::unique_ptr<Atom> m_last_value;
    std::map<AtomIdentifier, size_t> m_load_counts;
};

Program translate(hir::Program program, CompilationContext& context);
void print_program(const Program& program);

}  // namespace lir
