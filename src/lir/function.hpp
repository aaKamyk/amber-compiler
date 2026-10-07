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
    [[nodiscard]] bool has_terminator() const;
    [[nodiscard]] bool has_label() const;
    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] bool empty() const;

    [[nodiscard]] const std::string& get_label() const;
};

struct VariableInfo {
    enum class Kind : uint8_t {
        GLOBAL,
        LOCAL,
        PARAMETER,
        HIR_TEMPORARY,
        LIR_TEMPORARY,
    };
    AtomIdentifier identifier;
    Type type{Type::VOID};
    Kind kind{Kind::LIR_TEMPORARY};

    static VariableInfo from_argument(const hir::FunctionParameter& arg);
};

struct FunctionDefinition {
    std::unique_ptr<AtomIdentifier> name{nullptr};
    Type return_type{Type::VOID};
    std::vector<std::unique_ptr<BasicBlock>> blocks{};
    std::vector<VariableInfo> arguments{};
    std::vector<VariableInfo> variables{};

    explicit FunctionDefinition() = default;
    explicit FunctionDefinition(std::unique_ptr<AtomIdentifier> name, Type ret_type);

    [[nodiscard]] BasicBlock& current_block() const;
    [[nodiscard]] bool has_appendable_block() const;
    void append(BasicBlock basic_block);
    void append(std::unique_ptr<Instruction> instruction) const;
    [[nodiscard]] const std::string& get_name() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool has_variable(const AtomIdentifier& ident) const;
    [[nodiscard]] bool has_argument(const AtomIdentifier& ident) const;
    VariableInfo& get_variable(const AtomIdentifier& ident);
};

struct Module {
    std::vector<std::unique_ptr<FunctionDefinition>> functions;
};

}  // namespace lir
