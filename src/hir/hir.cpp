#include "hir.hpp"

#include <cassert>
#include <iostream>

#include "utils/symbolTable.hpp"

bool hir::Scope::contains(const uint64_t symbol) const {
    return symbols.contains(symbol);
}
void hir::Scope::add(const uint64_t symbol) {
    symbols.insert(symbol);
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
    /// TODO
    this->translate(std::move(assignment_statement->expression));
}

void hir::Hir::visit(ast::ReturnStatement* return_statement) {
    /// TODO
}

void hir::Hir::visit(ast::IfStatement* if_statement) {
    /// TODO
}

void hir::Hir::visit(ast::WhileStatement* while_statement) {
    /// TODO
}

void hir::Hir::visit(ast::VariableDeclaration* variable_declaration) {
    std::unique_ptr<AtomIdentifier> atom = std::move(variable_declaration->name);
    if (m_symbol_table.has_symbol(atom->name)) {
        /// TODO allow for redeclaration in more nested scope
        return;
    }
    const auto id = m_symbol_table.add_symbol(atom->name);
    this->current_scope().add(id);
    auto var_decl = std::make_unique<InstructionVariableDeclaration>(
        variable_declaration->type, atom->clone()
    );
    if (m_active_function) {
        auto instruction_decl =
            std::make_unique<InstructionDeclaration>(std::move(var_decl));
        m_active_function->emplace_back(std::move(instruction_decl));
    } else {
        this->append_to_program({std::move(var_decl)});
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
    auto func_decl = std::make_unique<InstructionFunctionDeclaration>(
        function_declaration->return_type, std::move(function_declaration->name)
    );
    for (const auto& param : function_declaration->arguments) {
        assert(param);
        auto& [type, name] = *param;
        func_decl->parameters.emplace_back(type, std::move(name));
    }
    this->set_active_function(func_decl->instructions);
    this->append_to_program({std::move(func_decl)});
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
    const auto id = m_symbol_table.add_symbol(atom->name);
    this->current_scope().add(id);
    auto temp_decl =
        std::make_unique<InstructionVariableDeclaration>(Type::INT, atom->clone());
    auto instr_decl = std::make_unique<InstructionDeclaration>(std::move(temp_decl));
    m_active_function->emplace_back(std::move(instr_decl));
    return atom;
}

std::optional<uint64_t> hir::Hir::resolve_scope(const AtomIdentifier* atom) const {
    if (m_symbol_table.has_symbol(atom->name)) {
        const auto id = m_symbol_table.get_symbol_id(atom->name);
        if (const auto& cur_scope = this->current_scope(); cur_scope.contains(id)) {
            return cur_scope.depth;
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

void hir::Hir::set_active_function(std::vector<Instruction>& func_body) {
    this->m_active_function = &func_body;
}

void hir::Hir::unset_active_function() {
    this->m_active_function = nullptr;
}

void hir::Hir::append_to_active_func(Instruction i) const {
    this->m_active_function->emplace_back(std::move(i));
}

void hir::Hir::append_to_program(InstructionDeclaration decl) {
    this->m_program.declarations.emplace_back(std::move(decl));
}

std::unique_ptr<hir::Expression> hir::Hir::get_last_expression() {
    if (m_active_expressions.empty()) { return nullptr; }
    std::unique_ptr<Expression> expression = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return expression;
}

struct HirPrinter final : hir::ExpressionVisitor {
    void print(const hir::Program& program) {
        for (const auto& [decl] : program.declarations) {
            std::visit([this](auto&& d) -> void { this->print(*d); }, decl);
        }
    }

    void print(const hir::Instruction& instr) {
        std::visit([this](auto&& i) -> void { this->print(*i); }, instr.instruction);
    }

    void print(const hir::InstructionAssignment& assignment) {
        std::cout << assignment.identifier->name << " = ";
        assignment.value->accept(this);
        std::cout << ";\n";
    }

    static void print(const hir::InstructionDeclaration& declaration) {
        if (auto* var_decl =
                std::get_if<std::unique_ptr<hir::InstructionVariableDeclaration>>(
                    &declaration.decl
                )) {
            print(**var_decl);
        } else {
            throw std::runtime_error("Unexpected function declaration");
        }
    }

    static void print(const hir::InstructionReturn& assignment) {}

    static void print(const hir::InstructionBranch& assignment) {}

    static void print(const hir::InstructionBranchConditional& assignment) {}

    static void print(const hir::InstructionVariableDeclaration& decl) {
        std::cout << decl.type << ' ';
        std::cout << decl.identifier->name << ";\n";
    }

    void print(const hir::InstructionFunctionDeclaration& decl) {
        std::cout << "fn ";
        std::cout << decl.name->name;
        std::cout << '(';
        for (const auto& [type, name] : decl.parameters) {
            std::cout << type << ' ';
            std::cout << name->name << ", ";
        }
        std::cout << ") -> ";
        std::cout << decl.return_type << '\n';
        for (const auto& instr : decl.instructions) {
            std::cout << "  ";
            this->print(instr);
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
        /// TODO
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
