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

void lir::Lir::visit(hir::InstructionDiscard*) {}

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

        this->get_active_func()->current_block().add(std::move(instruction_store));
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
    this->get_active_func()->current_block().add(std::move(instruction));
}

void lir::Lir::visit(hir::InstructionBranch* instruction_branch) {}
void lir::Lir::visit(hir::InstructionBranchConditional* instruction_branch_conditional) {}

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
    this->get_active_func()->current_block().add(std::move(instruction_alloca));
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

    /// construct definition from declaration
    this->m_lir_program.function_declarations.emplace_back(function);
    this->m_lir_program.functions.emplace_back(std::move(function));
    this->set_active_func(&this->m_lir_program.functions.back());
    for (const auto& instr : instruction_function_declaration->instructions) {
        instr->accept(this);
    }
}

void lir::Lir::visit(hir::BinaryExpression* binary_expression) {
    /// TODO type checking for operators
    static const std::map<hir::BinaryOperator, lir::BinaryIntegerOperator> mapping = {
        {hir::BinaryOperator::PLUS, lir::BinaryIntegerOperator::ADD},
        {hir::BinaryOperator::MINUS, lir::BinaryIntegerOperator::SUB},
        {hir::BinaryOperator::TIMES, lir::BinaryIntegerOperator::MUL},
        {hir::BinaryOperator::DIVIDE, lir::BinaryIntegerOperator::UDIV}
    };

    std::unique_ptr<AtomIdentifier> result_ident =
        this->m_target_value ? this->get_target_value()
                             : this->get_temporary_identifier("t");

    auto binary_instruction = std::make_unique<BinaryIntegerInstruction>();
    binary_instruction->result = result_ident->clone();
    binary_instruction->lhs = this->ensure_value(std::move(binary_expression->left));
    binary_instruction->rhs = this->ensure_value(std::move(binary_expression->right));
    binary_instruction->type = Type::I32;
    binary_instruction->op = mapping.at(binary_expression->op);

    this->get_active_func()->current_block().add(std::move(binary_instruction));
    this->set_last_value(std::move(result_ident));
}

void lir::Lir::visit(hir::UnaryExpression* unary_expression) {}

void lir::Lir::visit(hir::ComparisonExpression* comparison_expression) {}

void lir::Lir::visit(hir::TerminalExpression* terminal_expression) {
    this->set_last_value(std::move(terminal_expression->value));
}

void lir::Lir::visit(hir::CallExpression*) {}

void lir::Lir::set_active_func(FunctionDefinition* f) {
    this->m_active_func = f;
}

lir::FunctionDefinition* lir::Lir::get_active_func() const {
    return m_active_func;
}

std::unique_ptr<Atom> lir::Lir::ensure_value(std::unique_ptr<Atom> at) {
    if (dynamic_cast<AtomIntegerLiteral*>(at.get())) { return at; }
    if (const auto ident = dynamic_cast<AtomIdentifier*>(at.get())) {
        if (this->get_active_func()->has_argument(*ident)) { return at; }
        if (!this->get_active_func()->has_variable(*ident)) {
            throw std::runtime_error("Use of undeclared variable");
        }
        if (const auto& var_info = this->get_active_func()->get_variable(*ident);
            var_info.kind == FunctionDefinition::VariableInfo::Kind::LOCAL) {
            auto load_dest = std::make_unique<AtomIdentifier>(
                ident->name + ".load" + std::to_string(this->m_load_counts[*ident]++)
            );
            auto instruction_load = std::make_unique<LoadInstruction>(
                load_dest->clone(), std::move(at), Type::I32
            );
            this->get_active_func()->current_block().add(std::move(instruction_load));
            return load_dest;
        }
        return at;
    }
    throw std::invalid_argument("Invalid subclass of Atom");
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
    static void print(const Atom* at) {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            std::cout << "%" << ident->name;
        } else if (const auto* lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            std::cout << lit->value;
        }
    }

    void print(const std::unique_ptr<lir::BasicBlock>& bb) {
        std::cout << bb->label->name << ":\n";
        for (const auto& instr : bb->instructions) {
            std::cout << "    ";
            instr->accept(this);
        }
    }

    void print(const lir::Program& program) {
        /// Print declarations
        for (const auto& declaration : program.function_declarations) {
            std::cout << "declare " << declaration.return_type << " @"
                      << declaration.name->name << '(';
            if (const auto& types = declaration.argument_types; !types.empty()) {
                for (const auto& type : types | std::views::take(types.size() - 1)) {
                    std::cout << type << ", ";
                }
                std::cout << types.back();
            }
            std::cout << ")\n";
        }
        std::cout << '\n';
        /// Print definitions
        for (const auto& function : program.functions) {
            std::cout << "define " << function.return_type << " @" << function.name->name
                      << '(';
            if (const auto& args = function.arguments; !args.empty()) {
                for (const auto& [type, ident] :
                     args | std::views::take(args.size() - 1)) {
                    std::cout << type << " %";
                    std::cout << ident->name << ", ";
                }
                const auto& [type, ident] = args.back();
                std::cout << type << " %" << ident->name;
            }
            std::cout << ") {\n";
            if (const auto& blocks = function.blocks; !blocks.empty()) {
                for (const auto& bb : blocks | std::views::take(blocks.size() - 1)) {
                    print(bb);
                    std::cout << '\n';
                }
                print(blocks.back());
            }
            std::cout << "}\n\n";
        }
    }

    void visit(lir::AllocaInstruction* alloca_instruction) override {
        print(alloca_instruction->result.get());
        std::cout << " = alloca " << alloca_instruction->type << '\n';
    }

    void visit(lir::StoreInstruction* store_instruction) override {
        std::cout << "store " << store_instruction->type << ' ';
        print(store_instruction->value.get());
        std::cout << ", ptr ";
        print(store_instruction->destination.get());
        std::cout << '\n';
    }

    void visit(lir::LoadInstruction* load_instruction) override {
        print(load_instruction->result.get());
        std::cout << " = load " << load_instruction->type << ", ptr ";
        print(load_instruction->source.get());
        std::cout << '\n';
    }

    void visit(lir::BinaryIntegerInstruction* binary_integer_instruction) override {
        print(binary_integer_instruction->result.get());
        std::cout << " = " << binary_integer_instruction->op;
        std::cout << ' ' << binary_integer_instruction->type << ' ';
        print(binary_integer_instruction->lhs.get());
        std::cout << ", ";
        print(binary_integer_instruction->rhs.get());
        std::cout << '\n';
    }

    void visit(lir::BinaryFloatingInstruction*) override {}
    void visit(lir::BinaryBitwiseInstruction*) override {}

    void visit(lir::ReturnInstruction* return_instruction) override {
        std::cout << "ret " << return_instruction->type;
        if (return_instruction->type == lir::Type::VOID) {
            std::cout << '\n';
            return;
        }
        std::cout << ' ';
        print(return_instruction->value.get());
        std::cout << '\n';
    }
};

lir::Program lir::translate(hir::Program program, CompilationContext& context) {
    Lir translator(std::move(program));
    return translator.translate();
}

void lir::print_program(const Program& program) {
    LirPrinter printer{};
    printer.print(program);
}
