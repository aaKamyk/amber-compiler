#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "instruction.hpp"
#include "utils/type.hpp"

namespace hir {

class InstructionVisitor;

struct FunctionArgument {
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

    const std::string& get_name() const { return identifier->name; }
};

struct InstructionDiscard final : Instruction {
    std::unique_ptr<Expression> expression{nullptr};

    explicit InstructionDiscard(std::unique_ptr<Expression> expression);
    void accept(InstructionVisitor*) override;
};

struct InstructionAssignment final : Instruction {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::unique_ptr<Expression> value{nullptr};

    explicit InstructionAssignment(
        std::unique_ptr<AtomIdentifier> identifier, std::unique_ptr<Expression> value
    );
    void accept(InstructionVisitor*) override;

    const std::string& get_name() const { return identifier->name; }
};

/// value is set to nullptr if returning void
struct InstructionReturn final : Instruction {
    std::unique_ptr<TerminalExpression> value{nullptr};

    explicit InstructionReturn(std::unique_ptr<TerminalExpression> value);
    void accept(InstructionVisitor*) override;
};

struct InstructionBranch final : Instruction {
    std::unique_ptr<AtomIdentifier> destination{nullptr};

    explicit InstructionBranch(std::unique_ptr<AtomIdentifier> destination);
    void accept(InstructionVisitor*) override;

    const std::string& get_dest_name() const { return destination->name; }
};

struct InstructionBranchConditional final : Instruction {
    std::unique_ptr<ComparisonExpression> condition{nullptr};
    std::unique_ptr<AtomIdentifier> true_case{nullptr};
    std::unique_ptr<AtomIdentifier> false_case{nullptr};

    explicit InstructionBranchConditional() = default;
    explicit InstructionBranchConditional(
        std::unique_ptr<ComparisonExpression> condition,
        std::unique_ptr<AtomIdentifier> true_case,
        std::unique_ptr<AtomIdentifier> false_case
    );
    void accept(InstructionVisitor*) override;

    const std::string& get_tcase_name() const { return true_case->name; };
    const std::string& get_fcase_name() const { return false_case->name; };
};

struct InstructionDeclaration : Instruction {};

struct InstructionVariableDeclaration final : InstructionDeclaration {
    Type type{};
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    const bool is_temporary{false};

    explicit InstructionVariableDeclaration(
        Type type, std::unique_ptr<AtomIdentifier> identifier, bool is_temporary = false
    );
    void accept(InstructionVisitor*) override;

    const std::string& get_name() const { return identifier->name; }
};

struct InstructionFunctionDeclaration final : InstructionDeclaration {
    Type return_type{};
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::vector<FunctionArgument> parameters{};
    std::vector<std::unique_ptr<Instruction>> instructions{};

    explicit InstructionFunctionDeclaration() = default;
    explicit InstructionFunctionDeclaration(
        Type return_type, std::unique_ptr<AtomIdentifier> name,
        std::vector<FunctionArgument> parameters,
        std::vector<std::unique_ptr<Instruction>> instructions
    );
    void accept(InstructionVisitor*) override;

    const std::string& get_name() const { return name->name; }
};

class InstructionVisitor {
public:
    virtual void visit(Label*) = 0;

    virtual void visit(InstructionDiscard*) = 0;
    virtual void visit(InstructionAssignment*) = 0;
    virtual void visit(InstructionReturn*) = 0;
    virtual void visit(InstructionBranch*) = 0;
    virtual void visit(InstructionBranchConditional*) = 0;

    virtual void visit(InstructionVariableDeclaration*) = 0;
    virtual void visit(InstructionFunctionDeclaration*) = 0;

    virtual ~InstructionVisitor() = default;
};

}  // namespace hir
