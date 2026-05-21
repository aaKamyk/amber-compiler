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

struct FunctionDefinition {
    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{Type::VOID};
    std::vector<FunctionArgument> arguments{};
    std::vector<std::unique_ptr<BasicBlock>> blocks{};
    std::vector<AtomIdentifier> variables{};

    explicit FunctionDefinition() = default;
    explicit FunctionDefinition(std::unique_ptr<AtomIdentifier> name, Type ret_type);

    void append(BasicBlock basic_block);
    BasicBlock& current_block() const;
    bool empty() const;
    bool has_variable(const AtomIdentifier& ident) const;
    bool has_argument(const AtomIdentifier& ident) const;
};

struct FunctionDeclaration {
    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{Type::VOID};
    std::vector<Type> argument_types{};

    explicit FunctionDeclaration() = default;
    explicit FunctionDeclaration(const FunctionDefinition& definition);
};

struct Module {
    std::vector<std::unique_ptr<FunctionDefinition>> functions;
};

}  // namespace lir
