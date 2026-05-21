#pragma once

#include <memory>
#include <vector>

#include "hir/instruction.hpp"
#include "instruction.hpp"
#include "type.hpp"
#include "utils/atom.hpp"

namespace lir {

struct BasicBlock {
    std::unique_ptr<AtomIdentifier> label;
    std::vector<std::unique_ptr<Instruction>> instructions;

    void add(std::unique_ptr<Instruction> instruction);
};

struct FunctionArgument {
    Type type{};
    std::unique_ptr<AtomIdentifier> name{nullptr};

    explicit FunctionArgument(hir::FunctionArgument arg);
    explicit FunctionArgument(std::unique_ptr<AtomIdentifier> name, lir::Type type);
};

struct Function {
    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{};
    std::vector<FunctionArgument> arguments{};
    std::vector<std::unique_ptr<BasicBlock>> blocks{};
    std::vector<AtomIdentifier*> variables{};

    explicit Function() = default;
    explicit Function(std::unique_ptr<AtomIdentifier> name, Type ret_type);

    void append(BasicBlock basic_block);
    BasicBlock& current_block() const;
    bool empty() const;
};

struct Module {
    std::vector<std::unique_ptr<Function>> functions;
};

}  // namespace lir
