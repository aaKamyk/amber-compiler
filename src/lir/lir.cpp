#include "lir.hpp"

#include <iostream>
#include <map>
#include <ranges>

lir::Lir::Lir(hir::Program program)
    : m_program(std::move(program)), m_active_func(nullptr) {}

lir::Program lir::Lir::translate() {
    for (const auto& function : m_program.functions) { function->accept(this); };

    return std::move(m_lir_program);
}

void lir::Lir::visit(hir::Label* label) {
    if (!this->get_active_func()->empty()) {
        if (auto& last_block = this->get_active_func()->current_block();
            !last_block.has_terminator()) {
            auto instruction_branch = std::make_unique<BranchInstruction>();
            instruction_branch->true_case = label->identifier->clone();
            last_block.add(std::move(instruction_branch));
        }
    }
    BasicBlock bb;
    bb.label = std::move(label->identifier);
    this->get_active_func()->append(std::move(bb));
}

void lir::Lir::visit(hir::InstructionDiscard* instruction_discard) {
    this->m_discard_value = true;
    instruction_discard->expression->accept(this);
}

void lir::Lir::visit(hir::InstructionAssignment* instruction_assignment) {
    if (!this->get_active_func()->has_variable(*instruction_assignment->identifier)) {
        throw std::runtime_error("Use of undeclared variable");
    }
    const auto var_info =
        this->get_active_func()->get_variable(*instruction_assignment->identifier);

    if (var_info.kind == FunctionDefinition::VariableInfo::Kind::HIR_TEMPORARY) {
        this->set_target_value(std::move(instruction_assignment->identifier));
        instruction_assignment->value->accept(this);
    } else {
        instruction_assignment->value->accept(this);
        auto instruction_store = std::make_unique<StoreInstruction>();
        instruction_store->value = this->get_last_value();
        instruction_store->destination = std::move(instruction_assignment->identifier);
        instruction_store->type = Type::I32;

        this->get_active_func()->append(std::move(instruction_store));
    }
}

void lir::Lir::visit(hir::InstructionReturn* instruction_return) {
    auto instruction = std::make_unique<ReturnInstruction>();

    if (instruction_return->value) {
        instruction->type = Type::I32;
        instruction->value =
            this->ensure_value(std::move(instruction_return->value->value));
    } else {
        instruction->type = Type::VOID;
    }

    this->get_active_func()->append(std::move(instruction));
}

void lir::Lir::visit(hir::InstructionBranch* instruction_branch) {
    auto instruction = std::make_unique<BranchInstruction>();

    instruction->true_case = std::move(instruction_branch->destination);

    this->get_active_func()->append(std::move(instruction));
}

void lir::Lir::visit(hir::InstructionBranchConditional* instruction_branch_conditional) {
    auto instruction = std::make_unique<BranchInstruction>();

    instruction_branch_conditional->condition->accept(this);
    instruction->condition = this->get_last_value();
    instruction->true_case = std::move(instruction_branch_conditional->true_case);
    instruction->false_case = std::move(instruction_branch_conditional->false_case);

    this->get_active_func()->append(std::move(instruction));
}

void lir::Lir::visit(
    hir::InstructionVariableDeclaration* instruction_variable_declaration
) {
    this->get_active_func()->variables.push_back(
        {.name = *instruction_variable_declaration->identifier,
         .type = type_from(instruction_variable_declaration->type),
         .kind = instruction_variable_declaration->is_temporary
                     ? FunctionDefinition::VariableInfo::Kind::HIR_TEMPORARY
                     : FunctionDefinition::VariableInfo::Kind::LOCAL}
    );
    if (instruction_variable_declaration->is_temporary) { return; }

    auto instruction_alloca = std::make_unique<AllocaInstruction>(
        std::move(instruction_variable_declaration->identifier),
        type_from(instruction_variable_declaration->type)
    );

    this->get_active_func()->append(std::move(instruction_alloca));
}

void lir::Lir::visit(
    hir::InstructionFunctionDeclaration* instruction_function_declaration
) {
    FunctionDefinition function;

    function.return_type = type_from(instruction_function_declaration->return_type);
    function.name = std::move(instruction_function_declaration->name);

    auto& args = instruction_function_declaration->parameters;
    function.arguments.reserve(args.size());
    for (auto& argument : args) { function.arguments.emplace_back(std::move(argument)); }

    this->m_lir_program.functions.emplace_back(std::move(function));
    this->set_active_func(&this->m_lir_program.functions.back());
    for (const auto& instr : instruction_function_declaration->instructions) {
        if (!this->get_active_func()->has_appendable_block() &&
            !dynamic_cast<const hir::Label*>(instr.get())) {
            continue;
        }
        instr->accept(this);
    }
}

void lir::Lir::visit(hir::BinaryExpression* binary_expression) {
    /// TODO type checking for operators
    static const std::map<hir::BinaryOperator, lir::BinaryIntegerOperator> mapping = {
        {hir::BinaryOperator::PLUS, lir::BinaryIntegerOperator::ADD},
        {hir::BinaryOperator::MINUS, lir::BinaryIntegerOperator::SUB},
        {hir::BinaryOperator::TIMES, lir::BinaryIntegerOperator::MUL},
        {hir::BinaryOperator::DIVIDE, lir::BinaryIntegerOperator::SDIV}
    };

    std::unique_ptr<AtomIdentifier> result_ident =
        this->m_target_value ? this->get_target_value()
                             : this->get_temporary_identifier("t");

    auto binary_instruction = std::make_unique<BinaryIntegerInstruction>();
    binary_instruction->result = result_ident->clone();
    binary_instruction->lhs = this->ensure_value(std::move(binary_expression->left));
    binary_instruction->rhs = this->ensure_value(std::move(binary_expression->right));
    binary_instruction->operand_type = Type::I32;
    binary_instruction->op = mapping.at(binary_expression->op);

    this->get_active_func()->append(std::move(binary_instruction));
    this->set_last_value(std::move(result_ident));
}

void lir::Lir::visit(hir::UnaryExpression* unary_expression) {}

void lir::Lir::visit(hir::ComparisonExpression* comparison_expression) {
    static const std::map<hir::ComparisonOperator, lir::IntegerComparisonOperator>
        mapping = {
            {hir::ComparisonOperator::EQUAL, lir::IntegerComparisonOperator::EQ},
            {hir::ComparisonOperator::NOT_EQUAL, lir::IntegerComparisonOperator::NE},
            {hir::ComparisonOperator::GREATER, lir::IntegerComparisonOperator::SGT},
            {hir::ComparisonOperator::GREATER_EQUAL, lir::IntegerComparisonOperator::SGE},
            {hir::ComparisonOperator::LESS, lir::IntegerComparisonOperator::SLT},
            {hir::ComparisonOperator::LESS_EQUAL, lir::IntegerComparisonOperator::SLE}
        };

    std::unique_ptr<AtomIdentifier> result_ident =
        this->m_target_value ? this->get_target_value()
                             : this->get_temporary_identifier("t");

    auto comparison_instruction = std::make_unique<IntegerComparisonInstruction>();
    comparison_instruction->result = result_ident->clone();
    comparison_instruction->lhs =
        this->ensure_value(std::move(comparison_expression->left));
    comparison_instruction->rhs =
        this->ensure_value(std::move(comparison_expression->right));
    comparison_instruction->compared_type = Type::I32;
    comparison_instruction->op = mapping.at(comparison_expression->op);

    this->get_active_func()->append(std::move(comparison_instruction));
    this->set_last_value(std::move(result_ident));
}

void lir::Lir::visit(hir::TerminalExpression* terminal_expression) {
    this->set_last_value(std::move(terminal_expression->value));
}

void lir::Lir::visit(hir::CallExpression* call_expression) {
    auto call_instruction = std::make_unique<CallInstruction>();

    std::unique_ptr<AtomIdentifier> result_ident =
        this->m_discard_value  ? nullptr
        : this->m_target_value ? this->get_target_value()
                               : this->get_temporary_identifier("t");
    call_instruction->result = result_ident->clone();
    call_instruction->callee = std::move(call_expression->callee);
    call_instruction->arguments = std::move(call_expression->arguments);
    /// TODO match type to the function type
    call_instruction->type = Type::I32;

    this->get_active_func()->append(std::move(call_instruction));
    if (!this->m_discard_value) { this->set_last_value(std::move(result_ident)); }
}

void lir::Lir::set_active_func(FunctionDefinition* f) {
    this->m_active_func = f;
}

lir::FunctionDefinition* lir::Lir::get_active_func() const {
    return m_active_func;
}

std::unique_ptr<Atom> lir::Lir::ensure_value(std::unique_ptr<Atom> at) {
    if (dynamic_cast<AtomIntegerLiteral*>(at.get())) { return at; }

    const auto& ident = dynamic_cast<const AtomIdentifier&>(*at);
    if (this->get_active_func()->has_argument(ident)) { return at; }
    if (!this->get_active_func()->has_variable(ident)) {
        throw std::runtime_error("Use of undeclared variable");
    }
    if (const auto& var_info = this->get_active_func()->get_variable(ident);
        var_info.kind == FunctionDefinition::VariableInfo::Kind::LOCAL) {
        auto load_dest = std::make_unique<AtomIdentifier>(
            ident.name + ".load" + std::to_string(this->m_load_counts[ident]++)
        );
        auto instruction_load = std::make_unique<LoadInstruction>(
            load_dest->clone(), std::move(at), Type::I32
        );

        this->get_active_func()->append(std::move(instruction_load));
        return load_dest;
    }
    return at;
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_temporary_identifier(
    const std::string& name
) {
    static std::map<std::string, size_t> counts;
    std::string real_name = "__" + name + std::to_string(counts[name]++);
    return std::make_unique<AtomIdentifier>(real_name);
}

void lir::Lir::set_last_value(std::unique_ptr<Atom> at) {
    this->m_last_value = std::move(at);
}

void lir::Lir::set_target_value(std::unique_ptr<AtomIdentifier> at) {
    this->m_target_value = std::move(at);
}

std::unique_ptr<Atom> lir::Lir::get_last_value() {
    return std::move(m_last_value);
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_target_value() {
    return std::move(m_target_value);
}

struct LirPrinter final : lir::InstructionVisitor {
    static void print(const Atom* at, const bool local = true) {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            fmt::print("{}{}", local ? '%' : '@', ident->name);
        } else if (const auto* lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            fmt::print("{}", lit->value);
        }
    }

    void print(const std::unique_ptr<lir::BasicBlock>& bb) {
        fmt::println("{}:", bb->get_label());
        for (const auto& instr : bb->instructions) {
            fmt::print("    ");
            instr->accept(this);
        }
    }

    void print(const lir::Program& program) {
        /// Print definitions
        for (const auto& function : program.functions) {
            fmt::print("define {} @{}(", function.return_type, function.get_name());
            if (const auto& args = function.arguments; !args.empty()) {
                for (const auto& [type, ident] :
                     args | std::views::take(args.size() - 1)) {
                    fmt::print("{} %{}, ", type, ident->name);
                }
                const auto& [type, ident] = args.back();
                fmt::print("{} %{}", type, ident->name);
            }
            fmt::println(") {{");
            if (const auto& blocks = function.blocks; !blocks.empty()) {
                for (const auto& bb : blocks | std::views::take(blocks.size() - 1)) {
                    print(bb);
                    fmt::println("");
                }
                print(blocks.back());
            }
            fmt::print("}}\n\n");
        }
    }

    void visit(lir::AllocaInstruction* alloca_instruction) override {
        print(alloca_instruction->result.get());
        fmt::println(" = alloca {}", alloca_instruction->type);
    }

    void visit(lir::StoreInstruction* store_instruction) override {
        fmt::print("score {} ", store_instruction->type);
        print(store_instruction->value.get());
        fmt::print(", ptr");
        print(store_instruction->destination.get());
        fmt::println("");
    }

    void visit(lir::LoadInstruction* load_instruction) override {
        print(load_instruction->result.get());
        fmt::print(" = load {}, ptr ", load_instruction->type);
        print(load_instruction->source.get());
        fmt::println("");
    }

    void visit(lir::CallInstruction* call_instruction) override {
        if (call_instruction->result) {
            print(call_instruction->result.get());
            fmt::print(" = ");
        }
        fmt::print("call {} ", call_instruction->type);
        print(call_instruction->callee.get(), false);
        fmt::print("(");
        if (const auto& args = call_instruction->arguments; !args.empty()) {
            for (const auto& arg : args | std::views::take(args.size() - 1)) {
                print(arg.get());
                fmt::print(", ");
            }
            print(args.back().get());
        }
        fmt::println(")");
    }

    void visit(lir::BinaryIntegerInstruction* binary_integer_instruction) override {
        print(binary_integer_instruction->result.get());
        fmt::print(
            " = {} {} ", binary_integer_instruction->op,
            binary_integer_instruction->operand_type
        );
        print(binary_integer_instruction->lhs.get());
        fmt::print(", ");
        print(binary_integer_instruction->rhs.get());
        fmt::println("");
    }

    void visit(lir::BinaryFloatingPointInstruction*) override {}
    void visit(lir::BinaryBitwiseInstruction*) override {}

    void visit(lir::IntegerComparisonInstruction* comparison_instruction) override {
        print(comparison_instruction->result.get());
        fmt::print(
            " = icmp {} {} ", comparison_instruction->op,
            comparison_instruction->compared_type
        );
        print(comparison_instruction->lhs.get());
        fmt::print(", ");
        print(comparison_instruction->rhs.get());
        fmt::println("");
    }

    void visit(lir::FloatingPointComparisonInstruction* comparison_instruction) override {
        fmt::print(
            " = fcmp {} {} ", comparison_instruction->op,
            comparison_instruction->compared_type
        );
        print(comparison_instruction->lhs.get());
        fmt::print(", ");
        print(comparison_instruction->rhs.get());
        fmt::println("");
    }

    void visit(lir::ReturnInstruction* return_instruction) override {
        fmt::print("ret {}", return_instruction->type);
        if (return_instruction->type == lir::Type::VOID) {
            fmt::println("");
            return;
        }
        fmt::print(" ");
        print(return_instruction->value.get());
        fmt::println("");
    }

    void visit(lir::BranchInstruction* branch_instruction) override {
        fmt::print("br ");
        if (branch_instruction->condition) {
            fmt::print("i1 ");
            print(branch_instruction->condition.get());
            fmt::print(", label ");
            print(branch_instruction->true_case.get());
            fmt::print(", label ");
            print(branch_instruction->false_case.get());
        } else {
            fmt::print("label ");
            print(branch_instruction->true_case.get());
        }
        fmt::println("");
    }
};

lir::Program lir::translate(hir::Program program, CompilationContext& context) {
    Lir translator(std::move(program));
    return translator.translate();
}

void lir::print_program(const Program& program) {
    fmt::println("========== LIR ==========");
    LirPrinter printer{};
    printer.print(program);
}
