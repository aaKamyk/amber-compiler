#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "expression.hpp"
#include "utils/type.hpp"

namespace hir {

struct InstructionDeclaration;
struct InstructionVariableDeclaration;
struct InstructionFunctionDeclaration;
struct InstructionAssignment;
struct InstructionReturn;
struct InstructionBranch;
struct InstructionBranchConditional;

struct Argument {
    Type type;
    std::unique_ptr<AtomIdentifier> name;
};

struct Label {
    std::unique_ptr<AtomIdentifier> identifier;
};

struct Instruction {
    using Instr = std::variant<
        std::unique_ptr<InstructionAssignment>, std::unique_ptr<InstructionDeclaration>,
        std::unique_ptr<InstructionReturn>, std::unique_ptr<InstructionBranch>,
        std::unique_ptr<InstructionBranchConditional>>;
    Instr instruction;
};

struct InstructionDeclaration {
    using Decl = std::variant<
        std::unique_ptr<InstructionVariableDeclaration>,
        std::unique_ptr<InstructionFunctionDeclaration>>;
    Decl decl;
};

struct InstructionVariableDeclaration {
    Type type;
    std::unique_ptr<AtomIdentifier> identifier;
};

struct InstructionFunctionDeclaration {
    Type return_type;
    std::unique_ptr<AtomIdentifier> name;
    std::vector<Argument> parameters;
    std::vector<Instruction> instructions;
};

struct InstructionAssignment {
    std::unique_ptr<AtomIdentifier> identifier;
    std::unique_ptr<Expression> value;
};

struct InstructionReturn {
    std::unique_ptr<Atom> identifier;
};

struct InstructionBranch {
    std::unique_ptr<Label> destination;
};

struct InstructionBranchConditional {
    std::unique_ptr<ComparisonExpression> condition;
    std::unique_ptr<Label> true_case;
    std::unique_ptr<Label> false_case;
};

}  // namespace hir
