#include "pass.hpp"

#include "fmt/core.h"

void lir::ValidateBlocks::run(Program& program) {
    for (const auto& func : program.functions) {
        FunctionValidator validator(func, this->diagnostics);
        this->is_valid &= validator.validate();
    }
}

const std::vector<std::string>& lir::ValidateBlocks::get_diagnostics() const {
    return this->diagnostics;
}

void lir::ValidateBlocks::add_error(std::string msg) {
    this->diagnostics.emplace_back(std::move(msg));
}

bool lir::ValidateBlocks::FunctionValidator::validate() {
    this->collect_labels();
    auto valid{true};
    for (const auto& bb : func.blocks) { valid &= this->validate_block(*bb); }
    return valid;
}

void lir::ValidateBlocks::FunctionValidator::collect_labels() {
    for (const auto& bb : func.blocks) {
        if (bb->has_label()) { labels.push_back(bb->get_label()); }
    }
}

bool lir::ValidateBlocks::FunctionValidator::validate_block(const BasicBlock& bb) {
    auto valid{true};
    const bool has_label = bb.has_label();
    const std::string diagnostic_message = fmt::format(
        "Block {} in function {}", has_label ? bb.get_label() : "", func.get_name()
    );
    // label check
    if (!has_label) {
        valid = false;
        this->add_error(diagnostic_message + " does not have a label");
    }

    // empty check
    if (bb.empty()) {
        valid = false;
        this->add_error(diagnostic_message + " is empty");
        return valid;
    }

    // last instruction check
    if (!bb.has_terminator()) {
        valid = false;
        this->add_error(diagnostic_message + " does not have a well-placed terminator");
    }

    const auto& instructions = bb.instructions;
    // trailing instructions check
    for (size_t i = 0; i < instructions.size() - 1; ++i) {
        if (dynamic_cast<TerminatorInstruction*>(instructions[i].get()) != nullptr) {
            valid = false;
            this->add_error(
                diagnostic_message +
                " has early terminator at index {}, leaving dead instructions "
                "past it"
            );
            break;
        }
    }

    // branch label check
    for (const auto& instr : instructions) {
        if (const auto* br = dynamic_cast<const BranchInstruction*>(instr.get())) {
            if ((br->true_case && !validate_branch(*br)) ||
                (br->false_case && !validate_branch(*br))) {
                valid = false;
                this->add_error(
                    diagnostic_message + " has an branch to a non-existent label"
                );
                break;
            }
        }
    }
    return valid;
}

bool lir::ValidateBlocks::FunctionValidator::validate_branch(
    const BranchInstruction& br
) {
    const auto& labels = this->labels;
    if (std::ranges::find(this->labels, br.true_case->name) == this->labels.end()) {
        return false;
    }
    return !br.false_case ||
           std::ranges::find(this->labels, br.false_case->name) != this->labels.end();
}

void lir::ValidateBlocks::FunctionValidator::add_error(std::string msg) const {
    this->errs.emplace_back(std::move(msg));
}

void lir::diagnostic_pass(Program& program) {
    ValidateBlocks validator;
    validator.run(program);
    if (!validator.success()) {
        for (const auto& msg : validator.get_diagnostics()) { fmt::println("{}", msg); }
        std::abort();
    }
}

void lir::o0_pass(Program& program) {}