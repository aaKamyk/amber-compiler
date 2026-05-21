#include "function.hpp"

#include <algorithm>
#include <map>

namespace lir {

void BasicBlock::add(std::unique_ptr<Instruction> instruction) {
    this->instructions.emplace_back(std::move(instruction));
}

bool BasicBlock::has_terminator() const {
    if (this->instructions.empty()) return false;
    return dynamic_cast<const TerminatorInstruction*>(this->instructions.back().get());
}

bool BasicBlock::has_label() const {
    return this->label.get();
}

bool BasicBlock::is_valid() const {
    return this->has_label() && this->has_terminator();
}

FunctionArgument::FunctionArgument(hir::FunctionArgument arg) {
    this->type = type_from(arg.type);
    this->name = std::move(arg.name);
}
FunctionArgument::FunctionArgument(
    std::unique_ptr<AtomIdentifier> name, const lir::Type type
)
    : type(type), name(std::move(name)) {}

FunctionDefinition::FunctionDefinition(
    std::unique_ptr<AtomIdentifier> name, const lir::Type ret_type
)
    : name(std::move(name)), return_type(ret_type) {}

void FunctionDefinition::append(BasicBlock basic_block) {
    this->blocks.emplace_back(std::make_unique<BasicBlock>(std::move(basic_block)));
}

void FunctionDefinition::append(std::unique_ptr<Instruction> instruction) const {
    if (this->empty()) {
        throw std::runtime_error("Appending instruction with no basic blocks available");
    }
    this->current_block().add(std::move(instruction));
}

BasicBlock& FunctionDefinition::current_block() const {
    return *this->blocks.back();
}

bool FunctionDefinition::empty() const {
    return this->blocks.empty();
}

bool FunctionDefinition::has_variable(const AtomIdentifier& ident) const {
    return std::ranges::find_if(this->variables, [&](auto&& var_info) {
               return var_info.name == ident;
           }) != this->variables.end();
}

bool FunctionDefinition::has_argument(const AtomIdentifier& ident) const {
    return std::ranges::find_if(this->arguments, [&](auto&& arg) -> bool {
               return *arg.name == ident;
           }) != this->arguments.end();
}
FunctionDefinition::VariableInfo& FunctionDefinition::get_variable(
    const AtomIdentifier& ident
) {
    return *std::ranges::find_if(this->variables, [&](auto&& var_info) {
        return var_info.name == ident;
    });
}

}  // namespace lir