#pragma once

#include <memory>
#include <variant>

#include "expression.hpp"

namespace Hir {

struct InstructionDeclaration;
struct InstructionAssignment;
struct InstructionReturn;
struct InstructionBranch;
struct InstructionBranchConditional;

struct Instruction {
    using Instr = std::variant<
        std::unique_ptr<InstructionAssignment>, std::unique_ptr<InstructionDeclaration>,
        std::unique_ptr<InstructionReturn>, std::unique_ptr<InstructionBranch>,
        std::unique_ptr<InstructionBranchConditional>>;
    Instr instruction;
};

struct InstructionDeclaration {};

struct InstructionAssignment {};

struct InstructionReturn {};

struct InstructionBranch {};

struct InstructionBranchConditional {};

}  // namespace Hir
