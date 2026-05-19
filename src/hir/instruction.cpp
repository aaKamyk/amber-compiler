#include "instruction.hpp"

hir::Label::Label(std::unique_ptr<AtomIdentifier> identifier)
    : identifier(std::move(identifier)) {}

void hir::Label::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionAssignment::InstructionAssignment(
    std::unique_ptr<AtomIdentifier> identifier, std::unique_ptr<Expression> value
)
    : identifier(std::move(identifier)), value(std::move(value)) {}

void hir::InstructionAssignment::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionReturn::InstructionReturn(std::unique_ptr<Expression> value)
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
    const Type type, std::unique_ptr<AtomIdentifier> identifier
)
    : type(type), identifier(std::move(identifier)) {}

void hir::InstructionVariableDeclaration::accept(InstructionVisitor* v) {
    v->visit(this);
}

hir::InstructionFunctionDeclaration::InstructionFunctionDeclaration(
    const Type return_type, std::unique_ptr<AtomIdentifier> name,
    std::vector<Argument> parameters,
    std::vector<std::unique_ptr<Instruction>> instructions
)
    : return_type(return_type),
      name(std::move(name)),
      parameters(std::move(parameters)),
      instructions(std::move(instructions)) {}

void hir::InstructionFunctionDeclaration::accept(InstructionVisitor* v) {
    v->visit(this);
}