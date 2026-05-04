#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "instruction.hpp"
#include "utils/type.hpp"

namespace hir {

class InstructionVisitor;

struct Argument {
    Type type;
    std::unique_ptr<AtomIdentifier> name;
};

struct Instruction {
    virtual void accept(InstructionVisitor*) = 0;
    virtual ~Instruction() = default;
};

struct Label final : Instruction {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};

    explicit Label(std::unique_ptr<AtomIdentifier> identifier);
    void accept(InstructionVisitor*) override;
};

struct InstructionAssignment final : Instruction {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::unique_ptr<Expression> value{nullptr};

    explicit InstructionAssignment(
        std::unique_ptr<AtomIdentifier> identifier, std::unique_ptr<Expression> value
    );
    void accept(InstructionVisitor*) override;
};

struct InstructionReturn final : Instruction {
    std::unique_ptr<Expression> value{nullptr};

    explicit InstructionReturn(std::unique_ptr<Expression> value);
    void accept(InstructionVisitor*) override;
};

struct InstructionBranch final : Instruction {
    std::unique_ptr<Label> destination{nullptr};

    explicit InstructionBranch(std::unique_ptr<Label> destination);
    void accept(InstructionVisitor*) override;
};

struct InstructionBranchConditional final : Instruction {
    std::unique_ptr<ComparisonExpression> condition{nullptr};
    std::unique_ptr<Label> true_case{nullptr};
    std::unique_ptr<Label> false_case{nullptr};

    explicit InstructionBranchConditional(
        std::unique_ptr<ComparisonExpression> condition, std::unique_ptr<Label> true_case,
        std::unique_ptr<Label> false_case
    );
    void accept(InstructionVisitor*) override;
};

struct InstructionDeclaration : Instruction {};

struct InstructionVariableDeclaration final : InstructionDeclaration {
    Type type{};
    std::unique_ptr<AtomIdentifier> identifier{nullptr};

    explicit InstructionVariableDeclaration(
        Type type, std::unique_ptr<AtomIdentifier> identifier
    );
    void accept(InstructionVisitor*) override;
};

struct InstructionFunctionDeclaration final : InstructionDeclaration {
    Type return_type{};
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::vector<Argument> parameters{};
    std::vector<std::unique_ptr<Instruction>> instructions{};

    explicit InstructionFunctionDeclaration();
    explicit InstructionFunctionDeclaration(
        Type return_type, std::unique_ptr<AtomIdentifier> name,
        std::vector<Argument> parameters,
        std::vector<std::unique_ptr<Instruction>> instructions
    );
    void accept(InstructionVisitor*) override;
};

class InstructionVisitor {
public:
    virtual void visit(Label*) = 0;

    virtual void visit(InstructionAssignment*) = 0;
    virtual void visit(InstructionReturn*) = 0;
    virtual void visit(InstructionBranch*) = 0;
    virtual void visit(InstructionBranchConditional*) = 0;

    virtual void visit(InstructionVariableDeclaration*) = 0;
    virtual void visit(InstructionFunctionDeclaration*) = 0;

    virtual ~InstructionVisitor() = default;
};

}  // namespace hir
