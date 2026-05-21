#include "function.hpp"

#include <algorithm>
#include <map>

namespace lir {

void BasicBlock::add(std::unique_ptr<Instruction> instruction) {
    this->instructions.emplace_back(std::move(instruction));
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

BasicBlock& FunctionDefinition::current_block() const {
    return *this->blocks.back();
}

bool FunctionDefinition::empty() const {
    return this->blocks.empty();
}

bool FunctionDefinition::has_variable(const AtomIdentifier& ident) const {
    return std::ranges::find(this->variables, ident) != this->variables.end();
}

bool FunctionDefinition::has_argument(const AtomIdentifier& ident) const {
    return std::ranges::find_if(this->arguments, [&](auto&& arg) -> bool {
               return *arg.name == ident;
           }) != this->arguments.end();
}

FunctionDeclaration::FunctionDeclaration(const FunctionDefinition& definition)
    : name(definition.name->clone()), return_type(definition.return_type) {
    this->argument_types.reserve(definition.arguments.size());
    for (const auto& arg : definition.arguments) {
        argument_types.emplace_back(arg.type);
    }
}

}  // namespace lir