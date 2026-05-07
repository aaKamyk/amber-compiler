#include "hir.hpp"

#include <cassert>
#include <iostream>
#include <ranges>

#include "utils/symbolTable.hpp"

bool hir::Scope::contains(const NameID nid) const {
    return symbols.contains(nid);
}

SymbolID hir::Scope::get_sid(const NameID nid) const {
    return symbols.at(nid);
}

void hir::Scope::set(const NameID nid, const SymbolID sid) {
    symbols[nid] = sid;
}

hir::Hir::Hir(ast::Program program)
    : m_active_function(nullptr), m_ast_program(std::move(program)) {
    m_active_scopes.emplace_back(0);
}

hir::Program hir::Hir::translate() {
    for (auto& child : m_ast_program.children) { this->translate(std::move(child)); }
    return std::move(m_program);
}

void hir::Hir::visit(ast::BlockStatement* block_statement) {
    push_scope();
    for (auto& entry : block_statement->entries) { this->translate(std::move(entry)); }
    pop_scope();
}

void hir::Hir::visit(ast::AssignmentStatement* assignment_statement) {
    const auto scope_id = this->resolve_scope(assignment_statement->identifier.get());
    if (!scope_id) {
        /// TODO usage of undeclared variable
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
    this->translate(std::move(return_statement->expression));
    auto return_instruction =
        std::make_unique<InstructionReturn>(this->get_last_expression());
    m_active_function->emplace_back(std::move(return_instruction));
}

void hir::Hir::visit(ast::IfStatement* if_statement) {
    /// TODO
}

void hir::Hir::visit(ast::WhileStatement* while_statement) {
    /// TODO
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
    if (m_active_function) {
        m_active_function->emplace_back(std::move(var_decl));
    } else {
        this->append_to_program(std::move(var_decl));
    }
    /// Assignment
    if (variable_declaration->value) {
        this->translate(std::move(variable_declaration->value));
        if (m_active_function) {
            auto instr_assign = std::make_unique<InstructionAssignment>(
                std::move(atom), this->get_last_expression()
            );
            m_active_function->emplace_back(std::move(instr_assign));
            return;
        } else {
            /// TODO global scope assignment
        }
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
    this->translate(std::move(binary_expression->left));
    auto left = this->get_last_expression();
    this->translate(std::move(binary_expression->right));
    auto right = this->get_last_expression();

    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        m_active_function->emplace_back(std::move(assignment));
        return std::make_unique<Atom>(std::move(temp));
    };

    auto left_atom = ensure_atom(left);
    auto right_atom = ensure_atom(right);
    auto result = std::make_unique<BinaryExpression>(
        binary_expression->op, std::move(left_atom), std::move(right_atom)
    );
    m_active_expressions.emplace_back(std::move(result));
}

void hir::Hir::visit(ast::UnaryExpression* unary_expression) {
    this->translate(std::move(unary_expression->expression));
    auto last_expr = this->get_last_expression();

    auto ensure_atom = [this](std::unique_ptr<Expression>& e) -> std::unique_ptr<Atom> {
        if (auto* terminal = dynamic_cast<TerminalExpression*>(e.get())) {
            return std::move(terminal->value);
        }
        auto temp = this->create_temporary("v");
        auto assignment =
            std::make_unique<InstructionAssignment>(temp->clone(), std::move(e));
        m_active_function->emplace_back(std::move(assignment));
        return std::make_unique<Atom>(std::move(temp));
    };

    auto atom = ensure_atom(last_expr);
    auto result =
        std::make_unique<UnaryExpression>(unary_expression->op, std::move(atom));
    m_active_expressions.emplace_back(std::move(result));
}

std::unique_ptr<AtomIdentifier> hir::Hir::create_temporary(const std::string& name) {
    const uint64_t count = m_temporary_counts[name]++;
    std::string real_name = "__" + name + std::to_string(count);
    auto atom = std::make_unique<AtomIdentifier>(std::move(real_name));
    const auto nid = m_symbol_table.add_name(atom->name);
    /// TODO get a proper type checking in place
    const auto sid = m_symbol_table.add_symbol(
        {.type = Type::VOID, .nid = nid, .depth = this->current_scope().depth}
    );
    this->current_scope().set(nid, sid);
    auto temp_decl =
        std::make_unique<InstructionVariableDeclaration>(Type::INT, atom->clone());
    m_active_function->emplace_back(std::move(temp_decl));
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

void hir::Hir::append_to_active_func(std::unique_ptr<Instruction> i) const {
    this->m_active_function->emplace_back(std::move(i));
}

void hir::Hir::append_to_program(std::unique_ptr<InstructionDeclaration> decl) {
    this->m_program.declarations.emplace_back(std::move(decl));
}

std::unique_ptr<hir::Expression> hir::Hir::get_last_expression() {
    if (m_active_expressions.empty()) { return nullptr; }
    std::unique_ptr<Expression> expression = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return expression;
}

struct HirPrinter final : hir::ExpressionVisitor, hir::InstructionVisitor {
    void print(const hir::Program& program) {
        for (const auto& declaration : program.declarations) {
            declaration->accept(this);
        }
    }

    void visit(hir::Label* label) override {
        std::cout << label->identifier->name << ";\n";
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

    void visit(hir::InstructionBranch*) override {
        /// TODO
    }

    void visit(hir::InstructionBranchConditional*) override {
        /// TODO
    }

    void visit(hir::InstructionVariableDeclaration* instruction_declaration) override {
        std::cout << instruction_declaration->type << ' ';
        std::cout << instruction_declaration->identifier->name << ";\n";
    }

    void visit(hir::InstructionFunctionDeclaration* instruction_declaration) override {
        std::cout << "fn ";
        std::cout << instruction_declaration->name->name;
        std::cout << '(';
        for (const auto& [type, name] : instruction_declaration->parameters) {
            std::cout << type << ' ';
            std::cout << name->name << ", ";
        }
        std::cout << ") -> ";
        std::cout << instruction_declaration->return_type << '\n';
        for (const auto& instr : instruction_declaration->instructions) {
            std::cout << "  ";
            instr->accept(this);
        }
    }

    void visit(hir::BinaryExpression* binary_expression) override {
        print(*binary_expression->left);
        std::cout << ' ' << binary_expression->op << ' ';
        print(*binary_expression->right);
    }

    void visit(hir::UnaryExpression* unary_expression) override {
        std::cout << unary_expression->op;
        print(*unary_expression->operand);
    }

    void visit(hir::ComparisonExpression* comparison_expression) override {
        print(*comparison_expression->left);
        std::cout << ' ' << comparison_expression->op << ' ';
        print(*comparison_expression->right);
    }

    void visit(hir::TerminalExpression* terminal_expression) override {
        print(*terminal_expression->value);
    }

    static void print(const Atom& at) {
        if (const auto* ident = std::get_if<std::unique_ptr<AtomIdentifier>>(&at.atom)) {
            std::cout << (*ident)->name;
            return;
        }
        if (const auto* lit =
                std::get_if<std::unique_ptr<AtomIntegerLiteral>>(&at.atom)) {
            std::cout << (*lit)->value;
        }
    }
};

void hir::print_program(const Program& program) {
    HirPrinter printer{};
    printer.print(program);
}
