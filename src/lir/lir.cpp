#include "lir.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <ranges>

#include "fmt/ostream.h"
#include "pass.hpp"
#include "utils/util.hpp"

lir::Lir::Lir(hir::Program program)
    : m_program(std::move(program)), m_active_func(nullptr) {}

lir::Program lir::Lir::translate() {
    /// Register struct types
    for (const auto& [identifier, fields] :
         type::TypeRegistry::instance().type_ids() |
             std::views::transform([](const type::TypeID id) -> type::Type {
                 return type::TypeRegistry::instance().get_type(id);
             }) |
             std::views::filter([](const type::Type& type) -> bool {
                 return type.is<type::Struct>();
             }) |
             std::views::transform([](const type::Type& struct_type) -> type::Struct {
                 return struct_type.get<type::Struct>();
             })) {
        std::vector<std::unique_ptr<Type>> lir_fields{};
        lir_fields.reserve(fields.has_value() ? fields->size() : 0);
        std::ranges::transform(
            fields.value_or(std::vector<type::StructField>{}),
            std::back_inserter(lir_fields), [](const type::StructField& field) {
                return std::make_unique<Type>(type_from(field.type));
            }
        );

        StructureType lir_struct = {identifier.clone(), std::move(lir_fields)};
        TypeRegistry::definitions.insert({identifier.name, std::move(lir_struct)});
    }

    for (const auto& structure : TypeRegistry::definitions | std::views::values) {
        this->m_lir_program.structures.emplace_back(structure);
    }

    for (const auto& function : m_program.functions) { function->accept(this); };

    return std::move(this->m_lir_program);
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
    const auto& var_info =
        this->get_active_func()->get_variable(*instruction_assignment->identifier);

    if (var_info.kind == VariableInfo::Kind::HIR_TEMPORARY) {
        this->set_target_value(std::move(instruction_assignment->identifier));
        instruction_assignment->value->accept(this);
    } else {
        instruction_assignment->value->accept(this);
        auto instruction_store = std::make_unique<StoreInstruction>();
        instruction_store->value = this->get_last_value();
        instruction_store->destination =
            this->get_store_location(std::move(instruction_assignment->identifier));
        instruction_store->type = Type{PrimitiveType::I32};

        this->get_active_func()->append(std::move(instruction_store));
    }
}

void lir::Lir::visit(hir::InstructionReturn* instruction_return) {
    auto instruction = std::make_unique<ReturnInstruction>();

    if (instruction_return->value) {
        instruction->type = type_from(instruction_return->value->result_type);
        instruction->value =
            this->ensure_value(std::move(instruction_return->value->value));
    } else {
        instruction->type = Type{PrimitiveType::VOID};
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
    this->get_active_func()->variables.emplace_back(
        VariableInfo{
            .identifier = *instruction_variable_declaration->identifier,
            .type = type_from(instruction_variable_declaration->type),
            .kind = instruction_variable_declaration->is_temporary
                        ? VariableInfo::Kind::HIR_TEMPORARY
                        : VariableInfo::Kind::LOCAL
        }
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

    const auto& args = instruction_function_declaration->parameters;
    function.arguments.reserve(args.size());
    for (auto& argument : args) {
        function.arguments.emplace_back(VariableInfo::from_argument(argument));
    }
    this->m_lir_program.functions.emplace_back(std::move(function));
    this->set_active_func(&this->m_lir_program.functions.back());
    /// emit entry label
    instruction_function_declaration->entry_label->accept(this);
    for (const auto& param : this->get_active_func()->arguments) {
        this->allocate_parameter(param);
    }

    /// emit function body
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
                             : Lir::get_temporary_identifier("t");

    auto binary_instruction = std::make_unique<BinaryIntegerInstruction>();
    binary_instruction->result = result_ident->clone();
    binary_instruction->lhs = this->ensure_value(std::move(binary_expression->left));
    binary_instruction->rhs = this->ensure_value(std::move(binary_expression->right));
    binary_instruction->operand_type = type_from(binary_expression->result_type);
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
                             : Lir::get_temporary_identifier("t");

    auto comparison_instruction = std::make_unique<IntegerComparisonInstruction>();
    comparison_instruction->result = result_ident->clone();
    comparison_instruction->lhs =
        this->ensure_value(std::move(comparison_expression->left));
    comparison_instruction->rhs =
        this->ensure_value(std::move(comparison_expression->right));
    comparison_instruction->compared_type = type_from(comparison_expression->result_type);
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
                               : Lir::get_temporary_identifier("t");
    call_instruction->result = result_ident->clone();
    call_instruction->callee = std::move(call_expression->callee);
    for (auto& arg : call_expression->arguments) {
        call_instruction->arguments.push_back(
            {this->ensure_value(std::move(arg)), PrimitiveType::I32}
        );
    }

    call_instruction->type = type_from(call_expression->result_type);

    this->get_active_func()->append(std::move(call_instruction));
    if (!this->m_discard_value) { this->set_last_value(std::move(result_ident)); }
}

void lir::Lir::set_active_func(FunctionDefinition* f) {
    this->m_active_func = f;
}

lir::FunctionDefinition* lir::Lir::get_active_func() const {
    return m_active_func;
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_temporary_identifier(
    const std::string& name
) {
    static std::map<std::string, size_t> counts;
    std::string real_name = "__" + name + std::to_string(counts[name]++);
    return std::make_unique<AtomIdentifier>(real_name);
}

void lir::Lir::allocate_parameter(const VariableInfo& info) const {
    if (info.kind != VariableInfo::Kind::PARAMETER) {
        throw std::logic_error("Attempting to allocate parameter on a non-parameter");
    }
    std::string ptr_name = info.identifier.name + ".addr";

    /// allocate
    auto alloca_instr = std::make_unique<AllocaInstruction>(
        std::make_unique<AtomIdentifier>(ptr_name), info.type
    );
    this->get_active_func()->append(std::move(alloca_instr));

    // store
    auto store_instr = std::make_unique<StoreInstruction>();
    store_instr->value = std::make_unique<AtomIdentifier>(info.identifier);
    store_instr->destination = std::make_unique<AtomIdentifier>(ptr_name);
    store_instr->type = info.type;
    this->get_active_func()->append(std::move(store_instr));
}

void lir::Lir::set_last_value(std::unique_ptr<Atom> at) {
    this->m_last_value = std::move(at);
}

void lir::Lir::set_target_value(std::unique_ptr<AtomIdentifier> at) {
    this->m_target_value = std::move(at);
}

std::unique_ptr<Atom> lir::Lir::ensure_value(std::unique_ptr<Atom> at) {
    if (dynamic_cast<AtomIntegerLiteral*>(at.get())) { return at; }
    auto ident = dynamic_unique_cast<AtomIdentifier>(at);
    return this->ensure_value(std::move(ident));
}

std::unique_ptr<AtomIdentifier> lir::Lir::ensure_value(
    std::unique_ptr<AtomIdentifier> identifier
) {
    /// load argument value
    if (this->get_active_func()->has_argument(*identifier)) {
        const auto& arg_info = this->get_active_func()->get_variable(*identifier);
        auto load_dest = std::make_unique<AtomIdentifier>(
            arg_info.identifier.name + ".load" +
            std::to_string(this->m_load_counts[arg_info.identifier]++)
        );
        auto load_src =
            std::make_unique<AtomIdentifier>(arg_info.identifier.name + ".addr");
        auto instruction_load = std::make_unique<LoadInstruction>(
            load_dest->clone(), std::move(load_src), arg_info.type
        );
        this->get_active_func()->append(std::move(instruction_load));
        return load_dest;
    }
    if (!this->get_active_func()->has_variable(*identifier)) {
        throw std::runtime_error("Use of undeclared variable");
    }
    /// load variable value
    if (const auto& var_info = this->get_active_func()->get_variable(*identifier);
        var_info.kind == VariableInfo::Kind::LOCAL) {
        auto load_dest = std::make_unique<AtomIdentifier>(
            identifier->name + ".load" +
            std::to_string(this->m_load_counts[*identifier]++)
        );
        auto instruction_load = std::make_unique<LoadInstruction>(
            load_dest->clone(), std::move(identifier), var_info.type
        );

        this->get_active_func()->append(std::move(instruction_load));
        return load_dest;
    }
    return identifier;
}

std::unique_ptr<Atom> lir::Lir::get_last_value() {
    return std::move(m_last_value);
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_target_value() {
    return std::move(m_target_value);
}

std::unique_ptr<AtomIdentifier> lir::Lir::get_store_location(
    std::unique_ptr<AtomIdentifier> ident
) const {
    if (this->get_active_func()->has_argument(*ident)) {
        std::string ptr_name = ident->name + ".addr";
        return std::make_unique<AtomIdentifier>(std::move(ptr_name));
    }
    if (this->get_active_func()->has_variable(*ident)) { return ident; }
    throw std::runtime_error("Use of undeclared variable");
}

namespace {
struct LirPrinter final : lir::InstructionVisitor {
    std::ostream& out{std::cout};

    void print(const Atom* at, const bool local = true) const {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            fmt::print(this->out, "{}{}", local ? '%' : '@', ident->name);
        } else if (const auto* lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            fmt::print(this->out, "{}", lit->value);
        }
    }

    void print(const lir::Type& type) const {
        if (type.is<lir::StructureType>()) {
            fmt::print(this->out, "%{}", to_string(type.get<lir::StructureType>()));
        } else {
            fmt::print(this->out, "{}", type);
        }
    }

    void print(const std::unique_ptr<lir::BasicBlock>& bb) {
        fmt::println(this->out, "{}:", bb->get_label());
        for (const auto& instr : bb->instructions) {
            fmt::print(this->out, "    ");
            instr->accept(this);
        }
    }

    void print(const lir::Program& program) {
        /// Print type definitions
        for (const auto& structure : program.structures) {
            fmt::print(this->out, "%struct_{} = type {{ ", structure.identifier->name);
            for (const auto& field :
                 structure.fields | std::views::take(structure.fields.size() - 1)) {
                this->print(*field);
                fmt::print(this->out, ", ");
            }

            if (!structure.fields.empty()) {
                this->print(*structure.fields.back());
                fmt::println(this->out, " }}");
            } else {
                fmt::println(this->out, "}}");
            }
        }
        fmt::println(this->out, "");

        /// Print function definitions
        for (const auto& function : program.functions) {
            fmt::print(this->out, "define ");
            this->print(function.return_type);
            fmt::print(this->out, " @{}(", function.get_name());

            if (const auto& args = function.arguments; !args.empty()) {
                for (const auto& [ident, type, kind] :
                     args | std::views::take(args.size() - 1)) {
                    this->print(type);
                    fmt::print(this->out, " %{}, ", ident.name);
                }
                const auto& [ident, type, kind] = args.back();
                this->print(type);
                fmt::print(this->out, " %{}", ident.name);
            }
            fmt::println(this->out, ") {{");
            if (const auto& blocks = function.blocks; !blocks.empty()) {
                for (const auto& bb : blocks | std::views::take(blocks.size() - 1)) {
                    this->print(bb);
                    fmt::println(this->out, "");
                }
                this->print(blocks.back());
            }
            fmt::print(this->out, "}}\n\n");
        }
    }

    void visit(lir::AllocaInstruction* alloca_instruction) override {
        this->print(alloca_instruction->result.get());
        fmt::print(this->out, " = alloca ");
        this->print(alloca_instruction->type);
        fmt::println(this->out, "");
    }

    void visit(lir::StoreInstruction* store_instruction) override {
        fmt::print(this->out, "store {} ", store_instruction->type);
        this->print(store_instruction->value.get());
        fmt::print(this->out, ", ptr ");
        this->print(store_instruction->destination.get());
        fmt::println(this->out, "");
    }

    void visit(lir::LoadInstruction* load_instruction) override {
        print(load_instruction->result.get());
        fmt::print(this->out, " = load ");
        this->print(load_instruction->type);
        fmt::print(this->out, ", ptr ");
        print(load_instruction->source.get());
        fmt::println(this->out, "");
    }

    void visit(lir::GetElementPtrInstruction* get_element_ptr_instruction) override {
        this->print(get_element_ptr_instruction->result.get());
        fmt::print(this->out, " = getelementptr ");
        this->print(get_element_ptr_instruction->type);
        fmt::print(this->out, ", ptr ");
        print(get_element_ptr_instruction->source.get());
        fmt::print(this->out, ", ");
        for (const auto& idx : get_element_ptr_instruction->indices) {
            fmt::print(this->out, "i32 ");
            this->print(idx.get());
            fmt::print(this->out, ", ");
        }
        fmt::println(this->out, "");
    }

    void visit(lir::CallInstruction* call_instruction) override {
        if (call_instruction->result) {
            this->print(call_instruction->result.get());
            fmt::print(this->out, " = ");
        }
        fmt::print(this->out, "call ");
        this->print(call_instruction->type);
        fmt::print(this->out, " ");

        this->print(call_instruction->callee.get(), false);
        fmt::print(this->out, "(");
        if (const auto& args = call_instruction->arguments; !args.empty()) {
            for (const auto& [arg, type] : args | std::views::take(args.size() - 1)) {
                this->print(type);
                fmt::print(this->out, " ");
                this->print(arg.get());
                fmt::print(this->out, ", ");
            }
            const auto& [arg, type] = args.back();
            this->print(type);
            fmt::print(this->out, " ");
            this->print(arg.get());
        }
        fmt::println(this->out, ")");
    }

    void visit(lir::BinaryIntegerInstruction* binary_integer_instruction) override {
        this->print(binary_integer_instruction->result.get());
        fmt::print(this->out, " = {} ", binary_integer_instruction->op);
        this->print(binary_integer_instruction->operand_type);
        fmt::print(this->out, " ");
        this->print(binary_integer_instruction->lhs.get());
        fmt::print(this->out, ", ");
        this->print(binary_integer_instruction->rhs.get());
        fmt::println(this->out, "");
    }

    void visit(lir::BinaryFloatingPointInstruction*) override {}
    void visit(lir::BinaryBitwiseInstruction*) override {}

    void visit(lir::IntegerComparisonInstruction* comparison_instruction) override {
        this->print(comparison_instruction->result.get());
        fmt::print(this->out, " = icmp {} ", comparison_instruction->op);
        this->print(comparison_instruction->compared_type);
        fmt::print(this->out, " ");
        this->print(comparison_instruction->lhs.get());
        fmt::print(this->out, ", ");
        this->print(comparison_instruction->rhs.get());
        fmt::println(this->out, "");
    }

    void visit(lir::FloatingPointComparisonInstruction* comparison_instruction) override {
        fmt::print(this->out, " = fcmp {} ", comparison_instruction->op);
        this->print(comparison_instruction->compared_type);
        fmt::print(this->out, " ");
        this->print(comparison_instruction->lhs.get());
        fmt::print(this->out, ", ");
        this->print(comparison_instruction->rhs.get());
        fmt::println(this->out, "");
    }

    void visit(lir::ReturnInstruction* return_instruction) override {
        fmt::print(this->out, "ret ");
        this->print(return_instruction->type);

        if (return_instruction->type == lir::Type{lir::PrimitiveType::VOID}) {
            fmt::println(this->out, "");
            return;
        }

        fmt::print(this->out, " ");
        this->print(return_instruction->value.get());
        fmt::println(this->out, "");
    }

    void visit(lir::BranchInstruction* branch_instruction) override {
        fmt::print(this->out, "br ");
        if (branch_instruction->condition) {
            fmt::print(this->out, "i1 ");
            this->print(branch_instruction->condition.get());
            fmt::print(this->out, ", label ");
            this->print(branch_instruction->true_case.get());
            fmt::print(this->out, ", label ");
            this->print(branch_instruction->false_case.get());
        } else {
            fmt::print(this->out, "label ");
            this->print(branch_instruction->true_case.get());
        }
        fmt::println(this->out, "");
    }

    explicit LirPrinter(std::ostream& out)
        : out{out} {};
};
}  // namespace

lir::Program lir::translate(hir::Program program, CompilationContext& context) {
    Lir translator(std::move(program));
    auto lir_program = translator.translate();
    diagnostic_pass(lir_program);
    return lir_program;
}

void lir::print_program(const Program& program, std::ostream& out) {
    LirPrinter printer{out};
    printer.print(program);
}
