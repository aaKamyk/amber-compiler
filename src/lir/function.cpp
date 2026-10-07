#include "function.hpp"

#include <algorithm>

namespace lir {

void BasicBlock::add(std::unique_ptr<Instruction> instruction) {
    this->instructions.emplace_back(std::move(instruction));
}

bool BasicBlock::has_terminator() const {
    if (this->instructions.empty()) { return false; }
    return dynamic_cast<const TerminatorInstruction*>(this->instructions.back().get()) !=
           nullptr;
}

bool BasicBlock::has_label() const {
    return this->label != nullptr;
}

bool BasicBlock::is_valid() const {
    return this->has_label() && this->has_terminator();
}

bool BasicBlock::empty() const {
    return this->instructions.empty();
}

const std::string& BasicBlock::get_label() const {
    if (!this->label) { throw std::runtime_error("Basic block does not have a label"); }
    return this->label->name;
}

VariableInfo VariableInfo::from_argument(const hir::FunctionParameter& arg) {
    return {
        .identifier = std::move(*arg.name),
        .type = type_from(arg.type),
        .kind = Kind::PARAMETER
    };
}

FunctionDefinition::FunctionDefinition(
    std::unique_ptr<AtomIdentifier> name, const lir::Type ret_type
)
    : name(std::move(name)), return_type(ret_type) {}

BasicBlock& FunctionDefinition::current_block() const {
    return *this->blocks.back();
}

bool FunctionDefinition::has_appendable_block() const {
    if (this->empty()) { return false; }
    return !this->current_block().has_terminator();
}

void FunctionDefinition::append(BasicBlock basic_block) {
    this->blocks.emplace_back(std::make_unique<BasicBlock>(std::move(basic_block)));
}

void FunctionDefinition::append(std::unique_ptr<Instruction> instruction) const {
    if (this->empty()) {
        throw std::runtime_error("Appending instruction with no basic blocks available");
    }
    this->current_block().add(std::move(instruction));
}

const std::string& FunctionDefinition::get_name() const {
    return this->name->name;
}

bool FunctionDefinition::empty() const {
    return this->blocks.empty();
}

bool FunctionDefinition::has_variable(const AtomIdentifier& ident) const {
    return std::ranges::find_if(
               this->arguments, [&ident](auto&& arg) { return arg.identifier == ident; }
           ) != this->arguments.end() ||
           std::ranges::find_if(this->variables, [&](auto&& var_info) {
               return var_info.identifier == ident;
           }) != this->variables.end();
}

bool FunctionDefinition::has_argument(const AtomIdentifier& ident) const {
    return std::ranges::find_if(this->arguments, [&](auto&& arg) -> bool {
               return arg.identifier == ident;
           }) != this->arguments.end();
}

VariableInfo& FunctionDefinition::get_variable(const AtomIdentifier& ident) {
    if (const auto found = std::ranges::find_if(
            this->arguments, [&ident](auto&& arg) { return arg.identifier == ident; }
        );
        found != this->arguments.end()) {
        return *found;
    }
    if (const auto found = std::ranges::find_if(
            this->variables, [&](auto&& var_info) { return var_info.identifier == ident; }
        );
        found != this->variables.end()) {
        return *found;
    }
    throw std::runtime_error("Trying to get metadata of undeclared variable");
}

}  // namespace lir