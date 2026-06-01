#include "instruction.hpp"

hir::Label::Label(std::unique_ptr<AtomIdentifier> identifier)
    : identifier(std::move(identifier)) {}

void hir::Label::accept(InstructionVisitor* v) {
    v->visit(this);
}
hir::InstructionDiscard::InstructionDiscard(std::unique_ptr<Expression> expression)
    : expression(std::move(expression)) {}

void hir::InstructionDiscard::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionAssignment::InstructionAssignment(
    std::unique_ptr<AtomIdentifier> identifier, std::unique_ptr<Expression> value
)
    : identifier(std::move(identifier)), value(std::move(value)) {}

void hir::InstructionAssignment::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionReturn::InstructionReturn(std::unique_ptr<TerminalExpression> value)
    : value(std::move(value)) {}

void hir::InstructionReturn::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionBranch::InstructionBranch(std::unique_ptr<AtomIdentifier> destination)
    : destination(std::move(destination)) {}

void hir::InstructionBranch::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionBranchConditional::InstructionBranchConditional(
    std::unique_ptr<ComparisonExpression> condition,
    std::unique_ptr<AtomIdentifier> true_case, std::unique_ptr<AtomIdentifier> false_case
)
    : condition(std::move(condition)),
      true_case(std::move(true_case)),
      false_case(std::move(false_case)) {}

void hir::InstructionBranchConditional::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionVariableDeclaration::InstructionVariableDeclaration(
    const TypeID type, std::unique_ptr<AtomIdentifier> identifier, const bool is_temporary
)
    : type(type), identifier(std::move(identifier)), is_temporary(is_temporary) {}

void hir::InstructionVariableDeclaration::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionFunctionDeclaration::InstructionFunctionDeclaration(
    const TypeID return_type, std::unique_ptr<AtomIdentifier> name,
    std::unique_ptr<Label> entry_label, std::vector<FunctionParameter> parameters,
    std::vector<std::unique_ptr<Instruction>> instructions
)
    : return_type(return_type),
      name(std::move(name)),
      entry_label(std::move(entry_label)),
      parameters(std::move(parameters)),
      instructions(std::move(instructions)) {}

void hir::InstructionFunctionDeclaration::accept(InstructionVisitor* v) {
    v->visit(this);
}