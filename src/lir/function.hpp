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
    bool has_terminator() const;
    bool has_label() const;
    bool is_valid() const;
};

struct FunctionArgument {
    Type type{};
    std::unique_ptr<AtomIdentifier> name{nullptr};

    explicit FunctionArgument(hir::FunctionArgument arg);
    explicit FunctionArgument(std::unique_ptr<AtomIdentifier> name, lir::Type type);
};

struct FunctionDefinition {
    struct VariableInfo {
        enum class Kind {
            GLOBAL,
            LOCAL,
            HIR_TEMPORARY,
            LIR_TEMPORARY,
        };
        AtomIdentifier name;
        Type type{Type::VOID};
        Kind kind{Kind::LIR_TEMPORARY};
    };

    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{Type::VOID};
    std::vector<FunctionArgument> arguments{};
    std::vector<std::unique_ptr<BasicBlock>> blocks{};
    std::vector<VariableInfo> variables{};

    explicit FunctionDefinition() = default;
    explicit FunctionDefinition(std::unique_ptr<AtomIdentifier> name, Type ret_type);

    BasicBlock& current_block() const;
    void append(BasicBlock basic_block);
    void append(std::unique_ptr<Instruction> instruction) const;

    bool empty() const;
    bool has_variable(const AtomIdentifier& ident) const;
    bool has_argument(const AtomIdentifier& ident) const;
    VariableInfo& get_variable(const AtomIdentifier& ident);
};

struct FunctionDeclaration {
    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{Type::VOID};
    std::vector<Type> argument_types{};

    explicit FunctionDeclaration() = default;
};

struct Module {
    std::vector<std::unique_ptr<FunctionDefinition>> functions;
};

}  // namespace lir
