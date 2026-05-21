#pragma once

#include <memory>

#include "operator.hpp"
#include "type.hpp"
#include "utils/atom.hpp"

namespace lir {

class InstructionVisitor;

struct Instruction {
    virtual void accept(InstructionVisitor*) = 0;
    virtual ~Instruction() = default;
};

struct AllocaInstruction final : Instruction {
    /// %ptr = alloca i32
    std::unique_ptr<AtomIdentifier> result;
    Type type;

    explicit AllocaInstruction(std::unique_ptr<AtomIdentifier> result, Type type);
    void accept(InstructionVisitor*) override;
};

struct StoreInstruction final : Instruction {
    /// store i32 3, ptr %dest
    std::unique_ptr<Atom> value;
    std::unique_ptr<AtomIdentifier> destination;
    Type type;

    void accept(InstructionVisitor*) override;
};

struct LoadInstruction final : Instruction {
    /// %result = load i32, ptr %source
    std::unique_ptr<AtomIdentifier> result;
    std::unique_ptr<Atom> source;
    Type type;

    explicit LoadInstruction(
        std::unique_ptr<AtomIdentifier> result, std::unique_ptr<Atom> source, Type type
    );
    void accept(InstructionVisitor*) override;
};

struct BinaryInstruction : Instruction {
    std::unique_ptr<AtomIdentifier> result;
    std::unique_ptr<Atom> lhs;
    std::unique_ptr<Atom> rhs;
};

struct BinaryIntegerInstruction final : BinaryInstruction {
    /// %result = add i32, %lhs, %rhs
    BinaryIntegerOperator op;
    Type type;

    void accept(InstructionVisitor*) override;
};

struct BinaryFloatingInstruction final : BinaryInstruction {
    /// %result = fadd float, %lhs, %rhs
    BinaryFloatingOperator op;
    Type type;

    void accept(InstructionVisitor*) override;
};

struct BinaryBitwiseInstruction final : BinaryInstruction {
    /// %result = and i32, %lhs, %rhs
    BinaryBitwiseOperator op;
    Type type;

    void accept(InstructionVisitor*) override;
};

struct TerminatorInstruction : Instruction {};

struct ReturnInstruction final : TerminatorInstruction {
    std::unique_ptr<Atom> value;
    Type type;

    void accept(InstructionVisitor*) override;
};

class InstructionVisitor {
public:
    virtual void visit(AllocaInstruction*) = 0;
    virtual void visit(StoreInstruction*) = 0;
    virtual void visit(LoadInstruction*) = 0;
    virtual void visit(BinaryIntegerInstruction*) = 0;
    virtual void visit(BinaryFloatingInstruction*) = 0;
    virtual void visit(BinaryBitwiseInstruction*) = 0;

    virtual void visit(ReturnInstruction*) = 0;

    virtual ~InstructionVisitor() = default;
};

}  // namespace lir
