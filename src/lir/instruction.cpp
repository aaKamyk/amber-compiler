#include "instruction.hpp"

namespace lir {

AllocaInstruction::AllocaInstruction(std::unique_ptr<AtomIdentifier> result, Type type)
    : result(std::move(result)), type(std::move(type)) {}

void AllocaInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void StoreInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

LoadInstruction::LoadInstruction(
    std::unique_ptr<AtomIdentifier> result, std::unique_ptr<Atom> source, Type type
)
    : result(std::move(result)), source(std::move(source)), type(std::move(type)) {}

void LoadInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}
void CallInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void BinaryIntegerInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void BinaryFloatingPointInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void BinaryBitwiseInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void IntegerComparisonInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void FloatingPointComparisonInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void ReturnInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

void BranchInstruction::accept(InstructionVisitor* v) {
    v->visit(this);
}

}  // namespace lir