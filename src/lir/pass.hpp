#pragma once

#include "lir.hpp"

namespace lir {

struct Pass {
    virtual void run(Program&) = 0;
    virtual ~Pass() = default;
};

template <typename... Passes>
    requires std::is_base_of_v<Pass, Passes...>
void run_passes(Program& program) {
    (Passes{}.run(program), ...);
}

template <typename... Passes>
    requires(std::is_base_of_v<Pass, Passes> && ...)
void run_passes(Program& program, Passes... passes) {
    (passes.run(program), ...);
}

class ValidateBlocks final : Pass {
public:
    void run(Program& program) override;
    [[nodiscard]] bool success() const { return is_valid; }
    [[nodiscard]] const std::vector<std::string>& get_diagnostics() const;

private:
    bool is_valid{true};
    std::vector<std::string> diagnostics{};

    void add_error(std::string msg);

    struct FunctionValidator {
        FunctionValidator(const FunctionDefinition& func, std::vector<std::string>& errs)
            : func(func), errs(errs) {}

        bool validate();

    private:
        const FunctionDefinition& func;
        std::vector<std::string>& errs;
        std::vector<std::string> labels{};

        void collect_labels();
        bool validate_block(const BasicBlock& bb);
        bool validate_branch(const BranchInstruction& br);
        void add_error(std::string msg) const;
    };
};

void diagnostic_pass(Program& program);
void o0_pass(Program& program);
void o1_pass(Program& program);

}  // namespace lir
