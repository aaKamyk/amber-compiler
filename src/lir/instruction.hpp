#pragma once

#include <memory>
#include <vector>

#include "operator.hpp"
#include "type.hpp"
#include "utils/atom.hpp"

namespace lir {

class InstructionVisitor;

struct Instruction {
    virtual void accept(InstructionVisitor*) = 0;
    virtual ~Instruction() = default;
};

/// @code %ptr = alloca i32
struct AllocaInstruction final : Instruction {
    std::unique_ptr<AtomIdentifier> result;
    Type type;

    explicit AllocaInstruction(std::unique_ptr<AtomIdentifier> result, Type type);
    void accept(InstructionVisitor*) override;
};

/// @code store i32 3, ptr %dest
struct StoreInstruction final : Instruction {
    std::unique_ptr<Atom> value;
    std::unique_ptr<AtomIdentifier> destination;
    Type type;

    void accept(InstructionVisitor*) override;
};

/// @code %result = load i32, ptr %source
struct LoadInstruction final : Instruction {
    std::unique_ptr<AtomIdentifier> result;
    std::unique_ptr<Atom> source;
    Type type;

    explicit LoadInstruction(
        std::unique_ptr<AtomIdentifier> result, std::unique_ptr<Atom> source, Type type
    );
    void accept(InstructionVisitor*) override;
};

/// result is set to nullptr if return value is meant to be ignored \n
/// @code %result = call i32, @foo(%arg1, %arg2, ...) @endcode \n
/// @code call void @void_func()
struct CallInstruction final : Instruction {
    std::unique_ptr<AtomIdentifier> result{nullptr};
    std::unique_ptr<AtomIdentifier> callee{nullptr};
    std::vector<std::unique_ptr<Atom>> arguments;
    Type type;

    void accept(InstructionVisitor*) override;
};

struct BinaryInstruction : Instruction {
    std::unique_ptr<AtomIdentifier> result;
    std::unique_ptr<Atom> lhs;
    std::unique_ptr<Atom> rhs;
    Type operand_type;
};

/// @code %result = add i32, %lhs, %rhs
struct BinaryIntegerInstruction final : BinaryInstruction {
    BinaryIntegerOperator op;

    void accept(InstructionVisitor*) override;
};

/// @code %result = fadd float, %lhs, %rhs
struct BinaryFloatingPointInstruction final : BinaryInstruction {
    BinaryFloatingPointOperator op;

    void accept(InstructionVisitor*) override;
};

/// @code %result = and i32, %lhs, %rhs
struct BinaryBitwiseInstruction final : BinaryInstruction {
    BinaryBitwiseOperator op;

    void accept(InstructionVisitor*) override;
};

struct ComparisonInstruction : Instruction {
    std::unique_ptr<AtomIdentifier> result;
    std::unique_ptr<Atom> lhs;
    std::unique_ptr<Atom> rhs;
    Type compared_type;
};

/// @code %result = icmp eq i32 4, 5
struct IntegerComparisonInstruction final : ComparisonInstruction {
    IntegerComparisonOperator op;

    void accept(InstructionVisitor*) override;
};

/// @code %result = fcmp oeq float 4.0, 5.0
struct FloatingPointComparisonInstruction final : ComparisonInstruction {
    FloatingPointComparisonOperator op;

    void accept(InstructionVisitor*) override;
};

struct TerminatorInstruction : Instruction {};

/// value is set to nullptr if return type is void \n
/// @code ret i32 %x @endcode \n
/// @code ret void
struct ReturnInstruction final : TerminatorInstruction {
    std::unique_ptr<Atom> value;
    Type type;

    void accept(InstructionVisitor*) override;
};

/// if branch is unconditional condition is set to nullptr and true_case holds the
/// destination \n
/// @code br i1 %condition, label %if_true, label %if_false @endcode \n
/// @code br label %destination
struct BranchInstruction final : TerminatorInstruction {
    std::unique_ptr<Atom> condition{nullptr};
    std::unique_ptr<AtomIdentifier> true_case{nullptr};
    std::unique_ptr<AtomIdentifier> false_case{nullptr};

    void accept(InstructionVisitor*) override;
};

class InstructionVisitor {
public:
    virtual void visit(AllocaInstruction*) = 0;
    virtual void visit(StoreInstruction*) = 0;
    virtual void visit(LoadInstruction*) = 0;
    virtual void visit(CallInstruction*) = 0;

    virtual void visit(BinaryIntegerInstruction*) = 0;
    virtual void visit(BinaryFloatingPointInstruction*) = 0;
    virtual void visit(BinaryBitwiseInstruction*) = 0;

    virtual void visit(IntegerComparisonInstruction*) = 0;
    virtual void visit(FloatingPointComparisonInstruction*) = 0;

    virtual void visit(ReturnInstruction*) = 0;
    virtual void visit(BranchInstruction*) = 0;

    virtual ~InstructionVisitor() = default;
};

}  // namespace lir
