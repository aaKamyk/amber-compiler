#include "function.hpp"

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

Function::Function(std::unique_ptr<AtomIdentifier> name, const lir::Type ret_type)
    : name(std::move(name)), return_type(ret_type) {}

void Function::append(BasicBlock basic_block) {
    this->blocks.emplace_back(std::make_unique<BasicBlock>(std::move(basic_block)));
}

BasicBlock& Function::current_block() const {
    return *this->blocks.back();
}

bool Function::empty() const {
    return this->blocks.empty();
}

}  // namespace lir