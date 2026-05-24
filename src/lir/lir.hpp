#pragma once

#include "function.hpp"
#include "hir/expression.hpp"
#include "hir/hir.hpp"
#include "hir/instruction.hpp"
#include "utils/context.hpp"

namespace lir {

struct Program {
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
    std::unique_ptr<Atom> ensure_value(std::unique_ptr<Atom> at);
    static std::unique_ptr<AtomIdentifier> get_temporary_identifier(
        const std::string& name
    );

    void set_last_value(std::unique_ptr<Atom> at);
    void set_target_value(std::unique_ptr<AtomIdentifier> at);
    std::unique_ptr<Atom> get_last_value();
    std::unique_ptr<AtomIdentifier> get_target_value();

    hir::Program m_program;
    Program m_lir_program;
    FunctionDefinition* m_active_func;
    std::map<AtomIdentifier, size_t> m_load_counts;
    std::unique_ptr<AtomIdentifier> m_target_value{nullptr};
    std::unique_ptr<Atom> m_last_value{nullptr};
    bool m_discard_value{false};
};

Program translate(hir::Program program, CompilationContext& context);
void print_program(const Program& program);

}  // namespace lir
