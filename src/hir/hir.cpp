#include "hir.hpp"

#include <cassert>
#include <iostream>
#include <ranges>

#include "utils/symbolTable.hpp"
#include "utils/util.hpp"

bool hir::Scope::contains(const NameID nid) const {
    return symbols.contains(nid);
}

SymbolID hir::Scope::get_sid(const NameID nid) const {
    return symbols.at(nid);
}

void hir::Scope::set(const NameID nid, const SymbolID sid) {
    symbols[nid] = sid;
}

hir::Hir::Hir(ast::Program program, CompilationContext& ctx)
    : m_active_function(nullptr), m_ast_program(std::move(program)), m_ctx(ctx) {
    m_active_scopes.emplace_back(0);
}

hir::Program hir::Hir::translate() {
    for (auto& child : m_ast_program.children) { this->translate(std::move(child)); }
    if (!m_global_init.empty()) { this->append_global_init_to_program(); }
    return std::move(m_program);
}

bool hir::Hir::was_successful() const {
    return m_translated_successfully;
}

void hir::Hir::visit(ast::BlockStatement* block_statement) {
    push_scope();
    for (auto& entry : block_statement->entries) { this->translate(std::move(entry)); }
    pop_scope();
}

void hir::Hir::visit(ast::AssignmentStatement* assignment_statement) {
    const auto scope_id = this->resolve_scope(assignment_statement->identifier.get());
    if (!scope_id) {
        this->add_error(
            assignment_statement, "Symbol has not been declared in this scope \'" +
                                      assignment_statement->identifier->name + '\''
        );
        return;
    }
    // const auto nid =
    //     this->m_symbol_table.get_name_id(assignment_statement->identifier->name);
    // const auto sid = this->m_active_scopes[*scope_id].get_sid(nid);
    // const auto info = this->m_symbol_table.get_metadata(sid);
    this->translate(std::move(assignment_statement->expression));
    auto instruction_assignment = std::make_unique<InstructionAssignment>(
        std::move(assignment_statement->identifier), this->get_last_expression()
    );
    this->append_to_active_func(std::move(instruction_assignment));
}

void hir::Hir::visit(ast::ReturnStatement* return_statement) {
    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary_identifier("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        this->append_to_active_func(std::move(assignment));
        return temp;
    };

    this->translate(std::move(return_statement->expression));
    auto last_expr = this->get_last_expression();
    auto atom = ensure_atom(last_expr);
    auto return_instruction = std::make_unique<InstructionReturn>(
        std::make_unique<TerminalExpression>(std::move(atom))
    );
    this->append_to_active_func(std::move(return_instruction));
}

void hir::Hir::visit(ast::IfStatement* if_statement) {
    auto cond_branch = std::make_unique<InstructionBranchConditional>();
    this->translate(std::move(if_statement->condition));
    auto condition_expression = this->get_last_expression();
    /// create temporary variable before appending conditional branch instruction
    if (auto comparison_expr =
            dynamic_unique_cast<ComparisonExpression>(condition_expression)) {
        cond_branch->condition = std::move(comparison_expr);
    } else if (const auto terminal_expr =
                   dynamic_unique_cast<TerminalExpression>(condition_expression)) {
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            ComparisonOperator::EQUAL, std::move(terminal_expr->value),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    } else {
        auto cond_var = this->create_temporary_declaration("cond");
        auto instr_assign = std::make_unique<InstructionAssignment>(
            cond_var->clone(), std::move(condition_expression)
        );
        this->append_to_active_func(std::move(instr_assign));
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            ComparisonOperator::EQUAL, std::move(cond_var),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    }
    /// append to the function and get a reference to fill in the remaining fields
    this->append_to_active_func(std::move(cond_branch));
    auto& instr_branch =
        static_cast<InstructionBranchConditional&>(this->get_last_instruction());
    /// create a label, then translate all the block instructions
    auto true_label = this->create_temporary_label("case_true");
    this->translate(std::move(if_statement->true_case));
    instr_branch.true_case = std::move(true_label);
    if (if_statement->has_false_case()) {
        /// append a branch instruction to the end of the true block
        auto end_ident = this->create_temporary_identifier("if_end");
        this->append_to_active_func(
            std::make_unique<InstructionBranch>(end_ident->clone())
        );
        auto false_label = this->create_temporary_label("case_false");
        this->translate(std::move(if_statement->false_case));
        instr_branch.false_case = std::move(false_label);
        /// append the closing label to the end of the false block
        auto end_label = std::make_unique<Label>(std::move(end_ident));
        this->append_to_active_func(std::move(end_label));
    } else {
        auto end_label = this->create_temporary_identifier("if_end");
        this->append_to_active_func(std::make_unique<Label>(end_label->clone()));
        instr_branch.false_case = std::move(end_label);
    }
}

void hir::Hir::visit(ast::WhileStatement* while_statement) {
    auto while_cond = this->create_temporary_identifier("while_cond");
    auto cond_branch = std::make_unique<InstructionBranchConditional>();
    /// initial jump to condition
    this->append_to_active_func(std::make_unique<InstructionBranch>(while_cond->clone()));
    /// while body start
    auto while_body = this->create_temporary_label("while_body");
    this->translate(std::move(while_statement->block));
    /// while condition start
    this->append_to_active_func(std::make_unique<Label>(std::move(while_cond)));
    this->translate(std::move(while_statement->condition));
    auto condition_expression = this->get_last_expression();
    /// handle converting into a comparison expression
    if (auto comparison_expr =
            dynamic_unique_cast<ComparisonExpression>(condition_expression)) {
        cond_branch->condition = std::move(comparison_expr);
    } else if (const auto terminal_expr =
                   dynamic_unique_cast<TerminalExpression>(condition_expression)) {
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            ComparisonOperator::EQUAL, std::move(terminal_expr->value),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    } else {
        auto cond_var = this->create_temporary_declaration("cond");
        auto instr_assign = std::make_unique<InstructionAssignment>(
            cond_var->clone(), std::move(condition_expression)
        );
        this->append_to_active_func(std::move(instr_assign));
        cond_branch->condition = std::make_unique<ComparisonExpression>(
            ComparisonOperator::EQUAL, std::move(cond_var),
            std::make_unique<AtomIntegerLiteral>(0ull)
        );
    }
    /// append conditional branch
    this->append_to_active_func(std::move(cond_branch));
    auto& instr_branch =
        static_cast<InstructionBranchConditional&>(this->get_last_instruction());
    auto while_end = this->create_temporary_label("while_end");
    instr_branch.true_case = std::move(while_body);
    instr_branch.false_case = std::move(while_end);
}

void hir::Hir::visit(ast::ExpressionStatement* expression_statement) {
    this->translate(std::move(expression_statement->expression));
    auto expr = this->get_last_expression();
    if (dynamic_cast<CallExpression*>(expr.get())) {
        auto discard = std::make_unique<InstructionDiscard>(std::move(expr));
        this->append_to_active_func(std::move(discard));
        return;
    }
    /// TODO emit unused result of expression warning
}

void hir::Hir::visit(ast::VariableDeclaration* variable_declaration) {
    std::unique_ptr<AtomIdentifier> atom = std::move(variable_declaration->name);
    /// adds name to symbol table if didnt exist before
    const auto nid = m_symbol_table.add_name(atom->name);
    const auto sid = m_symbol_table.add_symbol(
        {.type = variable_declaration->type,
         .nid = nid,
         .depth = this->current_scope().depth}
    );
    /// add or shadow a new variable in the current scope
    this->current_scope().set(nid, sid);
    auto var_decl = std::make_unique<InstructionVariableDeclaration>(
        variable_declaration->type, atom->clone()
    );
    this->append_to_active_func(std::move(var_decl));
    /// assignment
    if (variable_declaration->value) {
        this->translate(std::move(variable_declaration->value));
        auto instr_assign = std::make_unique<InstructionAssignment>(
            std::move(atom), this->get_last_expression()
        );
        this->append_to_active_func(std::move(instr_assign));
    }
}

void hir::Hir::visit(ast::FunctionArgument* function_argument) {}

void hir::Hir::visit(ast::FunctionDeclaration* function_declaration) {
    if (current_scope().depth != 0) {
        /// TODO can only declare a function at global scope
        return;
    }
    auto func_decl = std::make_unique<InstructionFunctionDeclaration>();
    func_decl->return_type = function_declaration->return_type;
    func_decl->name = std::move(function_declaration->name);
    for (const auto& param : function_declaration->arguments) {
        assert(param);
        auto& [type, name] = *param;
        func_decl->parameters.emplace_back(type, std::move(name));
    }
    this->set_active_function(func_decl->instructions);
    this->append_to_active_func(std::make_unique<Label>(func_decl->name->clone()));
    this->append_to_program(std::move(func_decl));
    this->translate(std::move(function_declaration->body));
    this->unset_active_function();
}

void hir::Hir::visit(ast::AtomExpression* atom_expression) {
    m_active_expressions.emplace_back(
        std::make_unique<TerminalExpression>(std::move(atom_expression->atom))
    );
}

void hir::Hir::visit(ast::ParenExpression* paren_expression) {
    this->translate(std::move(paren_expression->expression));
}

void hir::Hir::visit(ast::BinaryExpression* binary_expression) {
    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary_identifier("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        this->append_to_active_func(std::move(assignment));
        return temp;
    };

    this->translate(std::move(binary_expression->left));
    auto left = this->get_last_expression();
    this->translate(std::move(binary_expression->right));
    auto right = this->get_last_expression();

    auto left_atom = ensure_atom(left);
    auto right_atom = ensure_atom(right);
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
    if (const auto it = arithmetic_operators.find(binary_expression->op);
        it != arithmetic_operators.end()) {
        auto result = std::make_unique<BinaryExpression>(
            it->second, std::move(left_atom), std::move(right_atom)
        );
        m_active_expressions.emplace_back(std::move(result));
        return;
    }
    if (const auto it = comparison_operators.find(binary_expression->op);
        it != comparison_operators.end()) {
        auto result = std::make_unique<ComparisonExpression>(
            it->second, std::move(left_atom), std::move(right_atom)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
}

void hir::Hir::visit(ast::UnaryExpression* unary_expression) {
    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary_identifier("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        this->append_to_active_func(std::move(assignment));
        return temp;
    };

    this->translate(std::move(unary_expression->expression));
    auto last_expr = this->get_last_expression();

    auto atom = ensure_atom(last_expr);
    std::map<ast::UnaryOperator, UnaryOperator> arithmetic_operators = {
        {ast::UnaryOperator::MINUS, UnaryOperator::MINUS},
    };
    std::map<ast::UnaryOperator, ComparisonOperator> logical_operators = {
        {ast::UnaryOperator::NOT, ComparisonOperator::NOT_EQUAL},
    };
    if (const auto it = arithmetic_operators.find(unary_expression->op);
        it != arithmetic_operators.end()) {
        auto result = std::make_unique<UnaryExpression>(it->second, std::move(atom));
        m_active_expressions.emplace_back(std::move(result));
        return;
    }
    if (const auto it = logical_operators.find(unary_expression->op);
        it != logical_operators.end()) {
        auto zero_lit = std::make_unique<AtomIntegerLiteral>(0);
        auto result = std::make_unique<ComparisonExpression>(
            it->second, std::move(atom), std::move(zero_lit)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
}

void hir::Hir::visit(ast::CallExpression* call_expression) {
    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary_identifier("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        this->append_to_active_func(std::move(assignment));
        return temp;
    };

    auto call_expr = std::make_unique<CallExpression>();
    call_expr->callee = std::move(call_expression->function);
    for (auto& arg : call_expression->arguments) {
        this->translate(std::move(arg));
        auto last_expr = this->get_last_expression();
        auto atom = ensure_atom(last_expr);
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
    const std::string& name
) {
    auto atom = this->create_temporary_identifier(name);
    const auto nid = m_symbol_table.add_name(atom->name);
    const auto sid = m_symbol_table.add_symbol(
        {.type = Type::VOID, .nid = nid, .depth = this->current_scope().depth}
    );
    this->current_scope().set(nid, sid);
    auto temp_decl =
        std::make_unique<InstructionVariableDeclaration>(Type::I32, atom->clone());
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

std::optional<uint64_t> hir::Hir::resolve_scope(const AtomIdentifier* atom) const {
    if (m_symbol_table.has_name(atom->name)) {
        const auto nid = m_symbol_table.get_name_id(atom->name);
        for (const auto& scope : std::views::reverse(m_active_scopes)) {
            if (scope.contains(nid)) { return scope.depth; }
        }
    }
    return std::nullopt;
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
    auto ident = std::make_unique<AtomIdentifier>("global_init");
    auto global_init = std::make_unique<InstructionFunctionDeclaration>(
        Type::VOID, std::move(ident), std::vector<Argument>{}, std::move(m_global_init)
    );
    this->append_to_program(std::move(global_init));
}

void hir::Hir::add_error(const ast::Node* error_node, const std::string& msg) {
    m_ctx.logic_errors().emplace_back(error_node->line, error_node->column, msg);
    m_translated_successfully = false;
}

std::unique_ptr<hir::Expression> hir::Hir::get_last_expression() {
    if (m_active_expressions.empty()) { return nullptr; }
    std::unique_ptr<Expression> expression = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return expression;
}

hir::Instruction& hir::Hir::get_last_instruction() const {
    if (m_active_function) {
        assert(!m_active_function->empty());
        return *m_active_function->back();
    }
    assert(!m_global_init.empty());
    return *m_global_init.back();
}

struct HirPrinter final : hir::ExpressionVisitor, hir::InstructionVisitor {
    void print(const hir::Program& program) {
        for (const auto& declaration : program.functions) { declaration->accept(this); }
    }

    void visit(hir::Label* label) override {
        std::cout << label->identifier->name << ":\n";
    };

    void visit(hir::InstructionDiscard* instruction_discard) override {
        instruction_discard->expression->accept(this);
        std::cout << ";\n";
    };

    void visit(hir::InstructionAssignment* instruction_assignment) override {
        std::cout << instruction_assignment->identifier->name << " = ";
        instruction_assignment->value->accept(this);
        std::cout << ";\n";
    }

    void visit(hir::InstructionReturn* instruction_return) override {
        std::cout << "return ";
        instruction_return->value->accept(this);
        std::cout << ";\n";
    }

    void visit(hir::InstructionBranch* instruction_branch) override {
        std::cout << "goto " << instruction_branch->destination->name << ";\n";
    }

    void visit(
        hir::InstructionBranchConditional* instruction_branch_conditional
    ) override {
        std::cout << "goto if ";
        instruction_branch_conditional->condition->accept(this);
        std::cout << " ";
        std::cout << instruction_branch_conditional->true_case->name << " : ";
        std::cout << instruction_branch_conditional->false_case->name << ";\n";
    }

    void visit(hir::InstructionVariableDeclaration* instruction_declaration) override {
        std::cout << instruction_declaration->type << ' ';
        std::cout << instruction_declaration->identifier->name << ";\n";
    }

    void visit(hir::InstructionFunctionDeclaration* instruction_declaration) override {
        std::cout << "fn ";
        std::cout << instruction_declaration->name->name;
        std::cout << '(';
        if (const auto& params = instruction_declaration->parameters; !params.empty()) {
            for (const auto& [type, ident] :
                 params | std::views::take(params.size() - 1)) {
                std::cout << type << ' ';
                std::cout << ident->name << ", ";
            }
            const auto& [type, ident] = params.back();
            std::cout << type << ' ' << ident->name;
        }
        std::cout << ") -> ";
        std::cout << instruction_declaration->return_type << '\n';
        for (const auto& instr : instruction_declaration->instructions) {
            if (!dynamic_cast<hir::Label*>(instr.get())) { std::cout << "  "; }
            instr->accept(this);
        }
        std::cout << '\n';
    }

    void visit(hir::BinaryExpression* binary_expression) override {
        print(binary_expression->left.get());
        std::cout << ' ' << binary_expression->op << ' ';
        print(binary_expression->right.get());
    }

    void visit(hir::UnaryExpression* unary_expression) override {
        std::cout << unary_expression->op;
        print(unary_expression->operand.get());
    }

    void visit(hir::ComparisonExpression* comparison_expression) override {
        print(comparison_expression->left.get());
        std::cout << ' ' << comparison_expression->op << ' ';
        print(comparison_expression->right.get());
    }

    void visit(hir::CallExpression* call_expression) override {
        std::cout << "call ";
        print(call_expression->callee.get());
        for (const auto& arg : call_expression->arguments) {
            std::cout << ' ';
            print(arg.get());
        }
    }

    void visit(hir::TerminalExpression* terminal_expression) override {
        print(terminal_expression->value.get());
    }

    static void print(const Atom* at) {
        if (const auto* ident = dynamic_cast<const AtomIdentifier*>(at)) {
            std::cout << ident->name;
        }
        if (const auto* lit = dynamic_cast<const AtomIntegerLiteral*>(at)) {
            std::cout << lit->value;
        }
    }
};

hir::Program hir::translate(ast::Program program, CompilationContext& ctx) {
    Hir translator(std::move(program), ctx);
    auto hir = translator.translate();
    if (!translator.was_successful()) {
        for (const auto& err : ctx.logic_errors()) {
            err.print(&ctx);
            std::cout << '\n';
        }
    }
    return hir;
}

void hir::print_program(const Program& program) {
    HirPrinter printer{};
    printer.print(program);
}
