#include "hir.hpp"

#include <cassert>
#include <format>
#include <iostream>
#include <ranges>

#include "fmt/core.h"
#include "fmt/ostream.h"
#include "utils/symbolTable.hpp"
#include "utils/util.hpp"

bool hir::Scope::contains(const std::string_view name) const {
    return symbols.contains(name.data());
}

void hir::Scope::add(std::string name, std::string unique_name) {
    this->symbols.insert({std::move(name), std::move(unique_name)});
}

std::string hir::Scope::get_unique_name(std::string_view name) const {
    if (symbols.contains(name.data())) { return symbols.at(name.data()); }
    throw std::out_of_range("Symbol not found");
}

hir::Hir::Hir(ast::Program<type::TypeID> program, CompilationContext& ctx)
    : m_active_function(nullptr), m_ast_program(std::move(program)), m_ctx(ctx) {
    m_active_scopes.emplace_back(0);
}

hir::Program hir::Hir::translate() {
    for (auto& child : m_ast_program.children) { this->translate(std::move(child)); }
    if (m_global_init.size() != 1) { this->append_global_init_to_program(); }
    return std::move(m_program);
}

bool hir::Hir::was_successful() const {
    return m_translated_successfully;
}

void hir::Hir::visit(ast::BlockStatement<type::TypeID>* block_statement) {
    push_scope();
    for (auto& entry : block_statement->entries) { this->translate(std::move(entry)); }
    pop_scope();
}

void hir::Hir::visit(ast::AssignmentStatement<type::TypeID>* assignment_statement) {
    auto unique_name = this->resolve_scope(*assignment_statement->identifier);
    if (!unique_name) {
        this->add_error(
            assignment_statement, "Symbol has not been declared in this scope \'" +
                                      assignment_statement->identifier->name + '\''
        );
        return;
    }
    const auto [type, is_mut] = this->m_symbol_table.get_metadata(*unique_name);

    this->translate(std::move(assignment_statement->expression));
    auto expr = this->get_last_expression();
    if (expr->result_type != type) {
        this->add_error(
            assignment_statement,
            fmt::format(
                "Cannot assign to variable of type {} with expression of type {}", type,
                expr->result_type
            )
        );
        return;
    }

    auto instruction_assignment = std::make_unique<InstructionAssignment>(
        std::make_unique<AtomIdentifier>(std::move(*unique_name)), std::move(expr)
    );
    this->append_to_active_func(std::move(instruction_assignment));
}

void hir::Hir::visit(ast::ReturnStatement<type::TypeID>* return_statement) {
    if (return_statement->expression) {
        this->translate(std::move(return_statement->expression));

        if (auto last_expr = this->get_last_expression();
            last_expr->result_type == type::unit()) {
            auto return_instruction = std::make_unique<InstructionReturn>(nullptr);
            this->append_to_active_func(std::move(return_instruction));
        } else {
            auto atom = this->ensure_atom(std::move(last_expr));
            auto return_instruction =
                std::make_unique<InstructionReturn>(std::make_unique<TerminalExpression>(
                    this->resolve_type(*atom), std::move(atom)
                ));
            this->append_to_active_func(std::move(return_instruction));
        }
    } else {
        auto return_instruction = std::make_unique<InstructionReturn>(nullptr);
        this->append_to_active_func(std::move(return_instruction));
    }
}

void hir::Hir::visit(ast::IfStatement<type::TypeID>* if_statement) {
    auto cond_branch = std::make_unique<InstructionBranchConditional>();

    this->translate(std::move(if_statement->condition));
    auto condition_expression = this->get_last_expression();

    /// If expression is a comparison keep it as is else reduce to an atom
    if (auto comparison_expr =
            dynamic_unique_cast<ComparisonExpression>(condition_expression)) {
        cond_branch->condition = std::move(comparison_expr);
    } else {
        /// Ensure the expression is an atom expression
        auto atom = [&]() -> std::unique_ptr<Atom> {
            if (auto* terminal_expr =
                    dynamic_cast<TerminalExpression*>(condition_expression.get())) {
                return std::move(terminal_expr->value);
            }

            auto cond_var = this->create_temporary_declaration(
                "cond", condition_expression->result_type
            );
            auto instr_assign = std::make_unique<InstructionAssignment>(
                cond_var->clone(), std::move(condition_expression)
            );
            this->append_to_active_func(std::move(instr_assign));
            return cond_var;
        }();
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            type::boolean(), ComparisonOperator::NOT_EQUAL, std::move(atom),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    }

    auto true_ident = this->create_temporary_identifier("case_true");
    auto end_ident = this->create_temporary_identifier("if_end");

    if (if_statement->has_false_case()) {
        auto false_ident = this->create_temporary_identifier("case_false");
        cond_branch->true_case = true_ident->clone();
        cond_branch->false_case = false_ident->clone();
        this->append_to_active_func(std::move(cond_branch));
        // true block
        this->append_to_active_func(std::make_unique<Label>(std::move(true_ident)));
        this->translate(std::move(if_statement->true_case));
        this->append_to_active_func(
            std::make_unique<InstructionBranch>(end_ident->clone())
        );
        // false block
        this->append_to_active_func(std::make_unique<Label>(std::move(false_ident)));
        this->translate(std::move(if_statement->false_case));
        // closing label
        this->append_to_active_func(std::make_unique<Label>(std::move(end_ident)));
    } else {
        cond_branch->true_case = true_ident->clone();
        cond_branch->false_case = end_ident->clone();
        this->append_to_active_func(std::move(cond_branch));
        // true block
        this->append_to_active_func(std::make_unique<Label>(std::move(true_ident)));
        this->translate(std::move(if_statement->true_case));
        // closing label
        this->append_to_active_func(std::make_unique<Label>(std::move(end_ident)));
    }
}

void hir::Hir::visit(ast::WhileStatement<type::TypeID>* while_statement) {
    auto cond_branch = std::make_unique<InstructionBranchConditional>();
    auto while_cond = this->create_temporary_identifier("while_cond");
    auto while_body = this->create_temporary_identifier("while_body");

    /// initial jump to condition
    this->append_to_active_func(std::make_unique<InstructionBranch>(while_cond->clone()));
    /// while body start
    this->append_to_active_func(std::make_unique<Label>(while_body->clone()));
    this->translate(std::move(while_statement->block));
    this->append_to_active_func(std::make_unique<InstructionBranch>(while_cond->clone()));
    /// while condition start
    this->append_to_active_func(std::make_unique<Label>(std::move(while_cond)));
    this->translate(std::move(while_statement->condition));
    auto condition_expression = this->get_last_expression();
    /// If expression is a comparison keep it as is else reduce to an atom
    if (auto comparison_expr =
            dynamic_unique_cast<ComparisonExpression>(condition_expression)) {
        cond_branch->condition = std::move(comparison_expr);
    } else {
        /// Ensure the expression is an atom expression
        auto atom = [&]() -> std::unique_ptr<Atom> {
            if (auto* terminal_expr =
                    dynamic_cast<TerminalExpression*>(condition_expression.get())) {
                return std::move(terminal_expr->value);
            }
            /// TODO change the default type
            auto cond_var = this->create_temporary_declaration("cond", type::i32());
            auto instr_assign = std::make_unique<InstructionAssignment>(
                cond_var->clone(), std::move(condition_expression)
            );
            this->append_to_active_func(std::move(instr_assign));
            return cond_var;
        }();
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            type::boolean(), ComparisonOperator::NOT_EQUAL, std::move(atom),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    }
    /// append conditional branch
    auto while_end = this->create_temporary_identifier("while_end");
    cond_branch->true_case = std::move(while_body);
    cond_branch->false_case = while_end->clone();
    this->append_to_active_func(std::move(cond_branch));
    this->append_to_active_func(std::make_unique<Label>(std::move(while_end)));
}

void hir::Hir::visit(ast::ExpressionStatement<type::TypeID>* expression_statement) {
    this->translate(std::move(expression_statement->expression));
    auto expr = this->get_last_expression();
    if (dynamic_cast<CallExpression*>(expr.get())) {
        auto discard = std::make_unique<InstructionDiscard>(std::move(expr));
        this->append_to_active_func(std::move(discard));
        return;
    }
    /// TODO emit unused result of expression warning
}

void hir::Hir::visit(ast::VariableDeclaration<type::TypeID>* variable_declaration) {
    /// check type against initializer expression
    type::TypeID variable_type{type::unit()};
    const bool has_initializer = variable_declaration->initializer != nullptr;
    if (has_initializer) {
        this->translate(std::move(variable_declaration->initializer));
        const auto* expr = this->view_last_expression();
        variable_type = expr->result_type;
        if (variable_declaration->type != type::inferred() &&
            variable_declaration->type != variable_type) {
            this->add_error(
                variable_declaration,
                fmt::format(
                    "Cannot assign to variable of type {} with expression of type {}",
                    variable_declaration->type, expr->result_type
                )
            );
        }
    } else {
        variable_type = variable_declaration->type;
    }
    /// gets the unique variable name and adds it to the symbol table
    auto& name = variable_declaration->name->name;
    auto unique_name = SymbolTable::get_unique_name(name);
    m_symbol_table.add_symbol(
        unique_name, SymbolMetadata{
                         .type = variable_type,
                         .is_mut = false,
                     }
    );
    /// add or shadow a new variable in the current scope
    this->current_scope().add(std::move(name), unique_name);
    auto name_atom = std::make_unique<AtomIdentifier>(std::move(unique_name));

    /// create declaration instruction
    auto var_decl = std::make_unique<InstructionVariableDeclaration>(
        variable_type, name_atom->clone()
    );
    this->append_to_active_func(std::move(var_decl));

    /// create assignment instruction
    if (has_initializer) {
        auto instr_assign = std::make_unique<InstructionAssignment>(
            std::move(name_atom), this->get_last_expression()
        );
        this->append_to_active_func(std::move(instr_assign));
    }
}

void hir::Hir::visit(ast::FunctionParameter<type::TypeID>* function_argument) {}

void hir::Hir::visit(ast::FunctionDeclaration<type::TypeID>* function_declaration) {
    if (current_scope().depth != 0) {
        /// TODO can only declare a function at global scope
        return;
    }
    auto func_decl = std::make_unique<InstructionFunctionDeclaration>();
    func_decl->return_type = function_declaration->return_type;
    func_decl->name = std::move(function_declaration->name);
    func_decl->entry_label = std::make_unique<Label>(func_decl->name->clone());
    /// additional scope for function parameters
    this->push_scope();
    for (const auto& param : function_declaration->arguments) {
        auto unique_name = SymbolTable::get_unique_name(param->name->name);
        m_symbol_table.add_symbol(
            unique_name, SymbolMetadata{
                             .type = param->type,
                             .is_mut = false,
                         }
        );
        this->current_scope().add(std::move(param->name->name), unique_name);
        func_decl->parameters.emplace_back(
            param->type, std::make_unique<AtomIdentifier>(std::move(unique_name))
        );
    }

    this->set_active_function(func_decl->instructions);
    this->append_to_program(std::move(func_decl));
    this->translate(std::move(function_declaration->body));
    this->pop_scope();
    this->unset_active_function();
}

void hir::Hir::visit(ast::StructField<type::TypeID>* struct_field) {}

void hir::Hir::visit(ast::StructDeclaration<type::TypeID>* struct_declaration) {}

void hir::Hir::visit(ast::AtomExpression* atom_expression) {
    if (const auto* ident =
            dynamic_cast<const AtomIdentifier*>(atom_expression->atom.get())) {
        auto unique_name = this->resolve_scope(*ident);
        if (!unique_name) { throw std::runtime_error("Usage of undeclared identifier"); }
        auto type = this->resolve_type(*unique_name);
        auto expr = std::make_unique<TerminalExpression>(
            type, std::make_unique<AtomIdentifier>(std::move(*unique_name))
        );
        m_active_expressions.emplace_back(std::move(expr));
    } else {
        auto expr = std::make_unique<TerminalExpression>(
            this->resolve_type(*atom_expression->atom), std::move(atom_expression->atom)
        );
        m_active_expressions.emplace_back(std::move(expr));
    }
}

void hir::Hir::visit(ast::ParenExpression* paren_expression) {
    this->translate(std::move(paren_expression->expression));
}

void hir::Hir::visit(ast::BinaryExpression* binary_expression) {
    this->translate(std::move(binary_expression->left));
    auto left_atom = this->ensure_atom(this->get_last_expression());
    this->translate(std::move(binary_expression->right));
    auto right_atom = this->ensure_atom(this->get_last_expression());

    std::map<ast::BinaryOperator, BinaryOperator> arithmetic_operators = {
        {ast::BinaryOperator::PLUS, BinaryOperator::PLUS},
        {ast::BinaryOperator::MINUS, BinaryOperator::MINUS},
        {ast::BinaryOperator::TIMES, BinaryOperator::TIMES},
        {ast::BinaryOperator::DIVIDE, BinaryOperator::DIVIDE},
    };
    std::map<ast::BinaryOperator, ComparisonOperator> comparison_operators = {

        {ast::BinaryOperator::EQUAL, ComparisonOperator::EQUAL},
        {ast::BinaryOperator::NOT_EQUAL, ComparisonOperator::NOT_EQUAL},
        {ast::BinaryOperator::LESS, ComparisonOperator::LESS},
        {ast::BinaryOperator::LESS_EQUAL, ComparisonOperator::LESS_EQUAL},
        {ast::BinaryOperator::GREATER, ComparisonOperator::GREATER},
        {ast::BinaryOperator::GREATER_EQUAL, ComparisonOperator::GREATER_EQUAL}
    };

    auto left_type = resolve_type(*left_atom);
    auto right_type = resolve_type(*right_atom);
    if (left_type != right_type) {
        /// TODO
    }
    if (const auto it = arithmetic_operators.find(binary_expression->op);
        it != arithmetic_operators.end()) {
        auto result = std::make_unique<BinaryExpression>(
            left_type, it->second, std::move(left_atom), std::move(right_atom)
        );
        m_active_expressions.emplace_back(std::move(result));
        return;
    }
    if (const auto it = comparison_operators.find(binary_expression->op);
        it != comparison_operators.end()) {
        auto result = std::make_unique<ComparisonExpression>(
            type::boolean(), it->second, std::move(left_atom), std::move(right_atom)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
}

void hir::Hir::visit(ast::UnaryExpression* unary_expression) {
    this->translate(std::move(unary_expression->expression));

    auto atom = this->ensure_atom(this->get_last_expression());
    std::map<ast::UnaryOperator, UnaryOperator> arithmetic_operators = {
        {ast::UnaryOperator::MINUS, UnaryOperator::MINUS},
    };
    std::map<ast::UnaryOperator, ComparisonOperator> logical_operators = {
        {ast::UnaryOperator::NOT, ComparisonOperator::NOT_EQUAL},
    };
    if (const auto it = arithmetic_operators.find(unary_expression->op);
        it != arithmetic_operators.end()) {
        auto result = std::make_unique<UnaryExpression>(
            this->resolve_type(*atom), it->second, std::move(atom)
        );
        m_active_expressions.emplace_back(std::move(result));
        return;
    }
    if (const auto it = logical_operators.find(unary_expression->op);
        it != logical_operators.end()) {
        auto zero_lit = std::make_unique<AtomIntegerLiteral>(0);
        auto result = std::make_unique<ComparisonExpression>(
            type::boolean(), it->second, std::move(atom), std::move(zero_lit)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
}

void hir::Hir::visit(ast::CallExpression* call_expression) {
    auto call_expr = std::make_unique<CallExpression>();

    const auto function_id =
        type::TypeRegistry::instance().get_type_id(call_expression->function->name);
    const auto function_type =
        type::TypeRegistry::instance().get_type(function_id).get<type::Function>();

    call_expr->result_type = function_type.return_type;
    call_expr->callee = std::move(call_expression->function);
    for (auto& arg : call_expression->arguments) {
        this->translate(std::move(arg));
        auto atom = this->ensure_atom(this->get_last_expression());
        call_expr->arguments.emplace_back(std::move(atom));
    }
    m_active_expressions.emplace_back(std::move(call_expr));
}

std::unique_ptr<AtomIdentifier> hir::Hir::create_temporary_identifier(
    const std::string& name
) {
    if (name == "_") { return std::make_unique<AtomIdentifier>(name); }
    const uint64_t count = m_temporary_counts[name]++;
    std::string real_name = "__" + name + std::to_string(count);
    return std::make_unique<AtomIdentifier>(std::move(real_name));
}

std::unique_ptr<AtomIdentifier> hir::Hir::create_temporary_declaration(
    const std::string& name, const type::TypeID type
) {
    auto atom = this->create_temporary_identifier(name);
    m_symbol_table.add_symbol(atom->name, {.type = type, .is_mut = false});
    this->current_scope().add(atom->name, atom->name);
    auto temp_decl =
        std::make_unique<InstructionVariableDeclaration>(type, atom->clone(), true);
    this->append_to_active_func(std::move(temp_decl));
    return atom;
}

std::unique_ptr<AtomIdentifier> hir::Hir::create_temporary_label(
    const std::string& name
) {
    auto atom = this->create_temporary_identifier(name);
    auto temp_label = std::make_unique<Label>(atom->clone());
    this->append_to_active_func(std::move(temp_label));
    return atom;
}

std::optional<std::string> hir::Hir::resolve_scope(const AtomIdentifier& atom) const {
    for (const auto& scope : std::views::reverse(m_active_scopes)) {
        if (scope.contains(atom.str())) { return scope.get_unique_name(atom.str()); }
    }
    return std::nullopt;
}

type::TypeID hir::Hir::resolve_type(const Atom& atom) const {
    if (dynamic_cast<const AtomIntegerLiteral*>(&atom)) { return type::i32(); }
    if (dynamic_cast<const AtomBooleanLiteral*>(&atom)) { return type::boolean(); }
    const auto& ident = dynamic_cast<const AtomIdentifier&>(atom);
    return this->m_symbol_table.get_metadata(ident.str()).type;
}

type::TypeID hir::Hir::resolve_type(const std::string& ident) const {
    return this->m_symbol_table.get_metadata(ident).type;
}

const hir::Scope& hir::Hir::current_scope() const {
    assert(!m_active_scopes.empty());
    return m_active_scopes.back();
}

hir::Scope& hir::Hir::current_scope() {
    assert(!m_active_scopes.empty());
    return m_active_scopes.back();
}

void hir::Hir::push_scope() {
    m_active_scopes.emplace_back(this->current_scope().depth + 1);
}

void hir::Hir::pop_scope() {
    assert(!m_active_scopes.empty());
    m_active_scopes.pop_back();
}

void hir::Hir::set_active_function(std::vector<std::unique_ptr<Instruction>>& func_body) {
    this->m_active_function = &func_body;
}

void hir::Hir::unset_active_function() {
    this->m_active_function = nullptr;
}

void hir::Hir::append_to_active_func(std::unique_ptr<Instruction> i) {
    if (m_active_function) {
        this->m_active_function->emplace_back(std::move(i));
    } else {
        this->m_global_init.emplace_back(std::move(i));
    }
}

void hir::Hir::append_to_program(std::unique_ptr<InstructionFunctionDeclaration> decl) {
    this->m_program.functions.emplace_back(std::move(decl));
}

void hir::Hir::append_global_init_to_program() {
    /// Add a return instruction at the end of the function
    auto return_instruction = std::make_unique<InstructionReturn>(nullptr);
    this->m_global_init.emplace_back(std::move(return_instruction));

    auto ident = std::make_unique<AtomIdentifier>("global_init");
    auto entry_label = std::make_unique<Label>(ident->clone());
    auto global_init = std::make_unique<InstructionFunctionDeclaration>(
        type::unit(), std::move(ident), std::move(entry_label),
        std::vector<FunctionParameter>{}, std::move(this->m_global_init)
    );
    this->append_to_program(std::move(global_init));
}

void hir::Hir::add_error(
    const ast::Node<type::TypeID>* error_node, const std::string& msg
) {
    m_ctx.logic_errors().emplace_back(error_node->line, error_node->column, msg);
    m_translated_successfully = false;
}

std::unique_ptr<hir::Expression> hir::Hir::get_last_expression() {
    if (m_active_expressions.empty()) { return nullptr; }
    std::unique_ptr<Expression> expression = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return expression;
}

const hir::Expression* hir::Hir::view_last_expression() const {
    if (m_active_expressions.empty()) { return nullptr; }
    return m_active_expressions.back().get();
}

std::unique_ptr<Atom> hir::Hir::ensure_atom(std::unique_ptr<Expression> expr) {
    if (auto* terminal = dynamic_cast<TerminalExpression*>(expr.get())) {
        return std::move(terminal->value);
    }

    auto temp = this->create_temporary_declaration("v", expr->result_type);
    auto assignment =
        std::make_unique<InstructionAssignment>(temp->clone(), std::move(expr));
    this->append_to_active_func(std::move(assignment));
    return temp;
}

hir::Instruction& hir::Hir::get_last_instruction() const {
    if (m_active_function) {
        assert(!m_active_function->empty());
        return *m_active_function->back();
    }
    assert(!m_global_init.empty());
    return *m_global_init.back();
}

namespace {
struct HirPrinter final : hir::ExpressionVisitor, hir::InstructionVisitor {
    std::ostream& out{std::cout};

    void print(const hir::Program& program) {
        for (const auto& declaration : program.functions) { declaration->accept(this); }
    }

    void visit(hir::Label* label) override {
        fmt::println(this->out, "{}:", label->get_name());
    };

    void visit(hir::InstructionDiscard* instruction_discard) override {
        instruction_discard->expression->accept(this);
        fmt::println(this->out, ";");
    };

    void visit(hir::InstructionAssignment* instruction_assignment) override {
        fmt::print(this->out, "{} = ", instruction_assignment->get_name());
        instruction_assignment->value->accept(this);
        fmt::println(this->out, ";");
    }

    void visit(hir::InstructionReturn* instruction_return) override {
        fmt::print(this->out, "return ");
        if (instruction_return->value) {
            instruction_return->value->accept(this);
        } else {
            fmt::print(this->out, "void");
        }
        fmt::println(this->out, ";");
    }

    void visit(hir::InstructionBranch* instruction_branch) override {
        fmt::println(this->out, "goto {};", instruction_branch->get_dest_name());
    }

    void visit(
        hir::InstructionBranchConditional* instruction_branch_conditional
    ) override {
        fmt::print(this->out, "goto if ");
        instruction_branch_conditional->condition->accept(this);
        fmt::println(
            this->out, " {} : {};", instruction_branch_conditional->get_tcase_name(),
            instruction_branch_conditional->get_fcase_name()
        );
    }

    void visit(hir::InstructionVariableDeclaration* instruction_declaration) override {
        fmt::println(
            this->out, "{} {};", instruction_declaration->type,
            instruction_declaration->get_name()
        );
    }

    void visit(hir::InstructionFunctionDeclaration* instruction_declaration) override {
        fmt::print(this->out, "fn {}(", instruction_declaration->get_name());
        if (const auto& params = instruction_declaration->parameters; !params.empty()) {
            for (const auto& [type, ident] :
                 params | std::views::take(params.size() - 1)) {
                fmt::print(this->out, "{} {}, ", type, ident->name);
            }
            const auto& [type, ident] = params.back();
            fmt::print(this->out, "{} {}", type, ident->name);
        }
        fmt::println(this->out, ") -> {}", instruction_declaration->return_type);
        fmt::println(this->out, "{}:", instruction_declaration->entry_label->get_name());
        for (const auto& instr : instruction_declaration->instructions) {
            if (!dynamic_cast<hir::Label*>(instr.get())) { fmt::print(this->out, "  "); }
            instr->accept(this);
        }
        fmt::println(this->out, "");
    }

    void visit(hir::BinaryExpression* binary_expression) override {
        print(binary_expression->left.get());
        fmt::print(this->out, " {} ", binary_expression->op);
        print(binary_expression->right.get());
    }

    void visit(hir::UnaryExpression* unary_expression) override {
        fmt::print(this->out, "{}", unary_expression->op);
        print(unary_expression->operand.get());
    }

    void visit(hir::ComparisonExpression* comparison_expression) override {
        print(comparison_expression->left.get());
        fmt::print(this->out, " {} ", comparison_expression->op);
        print(comparison_expression->right.get());
    }

    void visit(hir::CallExpression* call_expression) override {
        fmt::print(this->out, "call ");
        print(call_expression->callee.get());
        for (const auto& arg : call_expression->arguments) {
            fmt::print(this->out, " ");
            print(arg.get());
        }
    }

    void visit(hir::TerminalExpression* terminal_expression) override {
        print(terminal_expression->value.get());
    }

    void print(const Atom* at) const {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            fmt::print(this->out, "{}", ident->name);
        }
        if (const auto* int_lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            fmt::print(this->out, "{}", int_lit->value);
        }
        if (const auto* bool_lit = dynamic_cast<const AtomBooleanLiteral*>(at)) {
            fmt::print(this->out, "{}", bool_lit->value);
        }
    }

    explicit HirPrinter(std::ostream& out)
        : out(out) {};
};
}  // namespace

hir::Program hir::translate(ast::Program<type::TypeID> program, CompilationContext& ctx) {
    Hir translator(std::move(program), ctx);
    auto hir = translator.translate();
    if (!translator.was_successful()) {
        for (const auto& err : ctx.logic_errors()) {
            err.print(&ctx);
            fmt::println("");
        }
    }
    return hir;
}

void hir::print_program(const Program& program, std::ostream& out) {
    HirPrinter printer{out};
    printer.print(program);
}
