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
    BasicBlock bb;
    bb.label = std::move(label->identifier);
    this->m_active_func->append(std::move(bb));
}

void lir::Lir::visit(hir::InstructionDiscard*) {}

void lir::Lir::visit(hir::InstructionAssignment* instruction_assignment) {
    static std::map<AtomIdentifier, AtomIdentifier> loaded;
    if (!loaded.contains(*instruction_assignment->identifier)) {
        auto load_dest = std::make_unique<AtomIdentifier>(
            instruction_assignment->identifier->name + ".load"
        );
        loaded.insert({*instruction_assignment->identifier, *load_dest});
        auto instruction_load = std::make_unique<LoadInstruction>(
            std::move(load_dest), instruction_assignment->identifier->clone(), Type::I32
        );
        this->get_active_func()->current_block().add(std::move(instruction_load));
    }
    auto instruction_store = std::make_unique<StoreInstruction>();
    instruction_assignment->value->accept(this);
    instruction_store->value = this->get_last_value();
    instruction_store->destination =
        loaded.at(*instruction_assignment->identifier).clone();
    instruction_store->type = Type::I32;

    this->get_active_func()->current_block().add(std::move(instruction_store));
}

void lir::Lir::visit(hir::InstructionReturn* instruction_return) {
    auto instruction = std::make_unique<ReturnInstruction>();

    instruction->type = Type::I32;
    instruction_return->value->accept(this);
    instruction->value = this->get_last_value();

    this->get_active_func()->current_block().add(std::move(instruction));
}

void lir::Lir::visit(hir::InstructionBranch* instruction_branch) {}
void lir::Lir::visit(hir::InstructionBranchConditional* instruction_branch_conditional) {}

void lir::Lir::visit(
    hir::InstructionVariableDeclaration* instruction_variable_declaration
) {
    auto instruction_alloca = std::make_unique<AllocaInstruction>(
        std::move(instruction_variable_declaration->identifier),
        type_from(instruction_variable_declaration->type)
    );
    this->get_active_func()->current_block().add(std::move(instruction_alloca));
}

void lir::Lir::visit(
    hir::InstructionFunctionDeclaration* instruction_function_declaration
) {
    Function function;

    function.return_type = type_from(instruction_function_declaration->return_type);
    function.name = std::move(instruction_function_declaration->name);

    auto& args = instruction_function_declaration->parameters;
    function.arguments.reserve(args.size());
    for (auto& argument : args) { function.arguments.emplace_back(std::move(argument)); }

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
    auto temp = get_temporary_identifier("t");
    auto binary_instruction = std::make_unique<BinaryIntegerInstruction>();
    binary_instruction->result = temp->clone();
    binary_instruction->lhs = std::move(binary_expression->left);
    binary_instruction->rhs = std::move(binary_expression->right);
    binary_instruction->type = Type::I32;
    binary_instruction->op = mapping.at(binary_expression->op);

    this->get_active_func()->current_block().add(std::move(binary_instruction));
    this->m_last_value = std::move(temp);
}

void lir::Lir::visit(hir::UnaryExpression* unary_expression) {}

void lir::Lir::visit(hir::ComparisonExpression* comparison_expression) {}

void lir::Lir::visit(hir::TerminalExpression* terminal_expression) {
    m_last_value = std::move(terminal_expression->value);
}

void lir::Lir::visit(hir::CallExpression*) {}

void lir::Lir::set_active_func(Function* f) {
    this->m_active_func = f;
}

lir::Function* lir::Lir::get_active_func() const {
    return m_active_func;
}

std::unique_ptr<Atom> lir::Lir::get_last_value() {
    return std::move(m_last_value);
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_temporary_identifier(
    const std::string& name
) {
    static std::map<std::string, size_t> counts;
    std::string real_name = "__" + name + std::to_string(counts[name]);
    return std::make_unique<AtomIdentifier>(real_name);
}

struct LirPrinter final : lir::InstructionVisitor {
    static void print(const Atom* at) {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            std::cout << "%" << ident->name;
        } else if (const auto* lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            std::cout << lit->value;
        }
    }

    void print(const lir::Program& program) {
        for (const auto& function : program.functions) {
            std::cout << "define " << function.return_type << " @" << function.name->name
                      << "{\n";
            if (const auto& blocks = function.blocks; !blocks.empty()) {
                for (const auto& bb : blocks | std::views::take(blocks.size() - 1)) {
                    std::cout << bb->label->name << ":\n";
                    for (const auto& instr : bb->instructions) {
                        std::cout << "    ";
                        instr->accept(this);
                    }
                    std::cout << '\n';
                }
                const auto& bb = blocks.back();
                std::cout << bb->label->name << ":\n";
                for (const auto& instr : bb->instructions) {
                    std::cout << "    ";
                    instr->accept(this);
                }
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
        std::cout << ' ' << binary_integer_instruction->type << ", ";
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
