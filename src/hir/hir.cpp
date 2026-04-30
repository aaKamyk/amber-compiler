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

hir::Hir::Hir(ASTProgram program)
    : m_ast_program(std::move(program)), m_active_function(nullptr) {
    m_active_scopes.emplace_back(0);
}

hir::Program hir::Hir::translate() {
    for (auto& child : m_ast_program.children) {
        std::visit(
            [this]<typename T>(T&& node) -> void { this->translate(std::forward<T>(node)); },
            std::move(child)
        );
    }
    return std::move(m_program);
}

void hir::Hir::translate(std::unique_ptr<ASTExpression> expr) {
    std::visit(
        [this]<typename T>(T&& e) -> void { this->translate(std::forward<T>(e)); },
        std::move(expr->expression)
    );
}

void hir::Hir::translate(std::unique_ptr<ASTPrimaryExpression> expr) {
    std::visit(
        [this]<typename T>(T&& e) -> void { this->translate(std::forward<T>(e)); },
        std::move(expr->expression)
    );
}

void hir::Hir::translate(std::unique_ptr<ASTIdentifier> ident) {
    auto at = std::make_unique<Atom>(std::move(ident));
    m_active_expressions.emplace_back(std::make_unique<TerminalExpression>(std::move(at)));
}

void hir::Hir::translate(std::unique_ptr<ASTIntegerLiteral> lit) {
    auto at = std::make_unique<Atom>(std::move(lit));
    m_active_expressions.emplace_back(std::make_unique<TerminalExpression>(std::move(at)));
}

void hir::Hir::translate(std::unique_ptr<ASTParenExpression> expr) {
    this->translate(std::move(expr->expression));
}

void hir::Hir::translate(std::unique_ptr<ASTUnaryExpression> expr) {
    this->translate(std::move(expr->expression));
    if (this->is_last_expr_terminal()) {
        std::unique_ptr<Atom> atom = std::move(this->get_last_terminal_expr()->value);
        auto result = std::make_unique<UnaryExpression>(expr->op->op, std::move(atom));
        m_active_expressions.emplace_back(std::move(result));
        return;
    }
    auto temp_var = this->create_temporary("v");
    auto last_expr = this->get_last_expression();
    auto assign = std::make_unique<InstructionAssignment>(temp_var->clone(), std::move(last_expr));
    this->append_to_active_func({std::move(assign)});
    auto term_expr =
        std::make_unique<TerminalExpression>(std::make_unique<Atom>(std::move(temp_var)));
    m_active_expressions.emplace_back(std::move(term_expr));
}

void hir::Hir::translate(std::unique_ptr<ASTBinaryExpression> expr) {
    using TerminalExprPtr = std::unique_ptr<TerminalExpression>;
    this->translate(std::move(expr->left));
    auto left = this->get_last_expression();
    this->translate(std::move(expr->right));
    auto right = this->get_last_expression();

    std::unique_ptr<AtomIdentifier> temp_left;
    std::unique_ptr<AtomIdentifier> temp_right;
    if (!std::holds_alternative<TerminalExprPtr>(left->expression)) {
        temp_left = this->create_temporary("v");
        auto assign = std::make_unique<InstructionAssignment>(temp_left->clone(), std::move(left));
        m_active_function->emplace_back(std::move(assign));
    }
    if (!std::holds_alternative<TerminalExprPtr>(right->expression)) {
        temp_right = this->create_temporary("v");
        auto assign =
            std::make_unique<InstructionAssignment>(temp_right->clone(), std::move(right));
        m_active_function->emplace_back(std::move(assign));
    }
    if (left && right) {
        auto [left_atom] = std::move(*std::move(std::get<TerminalExprPtr>(left->expression)));
        auto [right_atom] = std::move(*std::move(std::get<TerminalExprPtr>(right->expression)));
        auto result = std::make_unique<BinaryExpression>(
            expr->op->op, std::move(left_atom), std::move(right_atom)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
    if (left && !right) {
        auto [atom] = std::move(*std::move(std::get<TerminalExprPtr>(left->expression)));
        auto result = std::make_unique<BinaryExpression>(
            expr->op->op, std::move(atom), std::make_unique<Atom>(std::move(temp_right))
        );
        m_active_expressions.emplace_back(std::move(result));
    }
    if (!left && right) {
        auto [atom] = std::move(*std::move(std::get<TerminalExprPtr>(right->expression)));
        auto result = std::make_unique<BinaryExpression>(
            expr->op->op, std::make_unique<Atom>(std::move(temp_left)), std::move(atom)
        );
        m_active_expressions.emplace_back(std::move(result));
    }
}

void hir::Hir::translate(std::unique_ptr<ASTDeclaration> decl) {
    std::visit(
        [this]<typename T>(T&& d) -> void { this->translate(std::forward<T>(d)); },
        std::move(decl->declaration)
    );
}

void hir::Hir::translate(std::unique_ptr<ASTVariableDeclaration> decl) {
    std::unique_ptr<AtomIdentifier> atom = std::move(decl->name);
    if (m_symbol_table.has_symbol(atom->name)) {
        /// TODO allow for redeclaration in more nested scope
        return;
    }
    const auto id = m_symbol_table.add_symbol(atom->name);
    this->current_scope().add(id);
    auto var_decl =
        std::make_unique<InstructionVariableDeclaration>(decl->type->type, atom->clone());
    if (m_active_function) {
        auto instruction_decl = std::make_unique<InstructionDeclaration>(std::move(var_decl));
        m_active_function->emplace_back(std::move(instruction_decl));
    } else {
        this->append_to_program({std::move(var_decl)});
    }
    /// Assignment
    if (decl->value) {
        this->translate(std::move(decl->value));
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

void hir::Hir::translate(std::unique_ptr<ASTFunctionDeclaration> decl) {
    if (current_scope().depth != 0) {
        /// TODO can only declare a function at global scope
        return;
    }
    auto func_decl = std::make_unique<InstructionFunctionDeclaration>(
        decl->return_type->type, std::move(decl->name)
    );
    for (const auto& param : decl->arguments) {
        assert(param);
        auto& [type, name] = *param;
        func_decl->parameters.emplace_back(type->type, std::move(name));
    }
    this->set_active_function(func_decl->instructions);
    this->append_to_program({std::move(func_decl)});
    this->translate(std::move(decl->body));
    this->unset_active_function();
}

void hir::Hir::translate(std::unique_ptr<ASTStatement> stmt) {
    std::visit(
        [this]<typename T>(T&& s) -> void { this->translate(std::forward<T>(s)); },
        std::move(stmt->statement)
    );
}

void hir::Hir::translate(std::unique_ptr<ASTBlockStatement> stmt) {
    push_scope();
    for (auto& entry : stmt->entries) {
        std::visit(
            [this]<typename T>(T&& s) -> void { this->translate(std::forward<T>(s)); },
            std::move(entry)
        );
    }
    pop_scope();
}

void hir::Hir::translate(std::unique_ptr<ASTAssignmentStatement> stmt) {
    this->translate(std::move(stmt->expression));
}

void hir::Hir::translate(std::unique_ptr<ASTIfStatement> stmt) {}

void hir::Hir::translate(std::unique_ptr<ASTWhileStatement> stmt) {}

void hir::Hir::translate(std::unique_ptr<ASTReturnStatement> stmt) {}

std::unique_ptr<AtomIdentifier> hir::Hir::create_temporary(const std::string_view name) {
    auto atom = std::make_unique<AtomIdentifier>(name.data());
    const auto id = m_symbol_table.add_symbol(atom->name);
    this->current_scope().add(id);
    auto temp_decl = std::make_unique<InstructionVariableDeclaration>(Type::INT, atom->clone());
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

void hir::Hir::append_to_active_func(Instruction i) {
    this->m_active_function->emplace_back(std::move(i));
}

void hir::Hir::append_to_program(InstructionDeclaration decl) {
    this->m_program.declarations.emplace_back(std::move(decl));
}

bool hir::Hir::is_last_expr_terminal() const {
    const auto& [expression] = m_active_expressions.back();
    return std::holds_alternative<std::unique_ptr<TerminalExpression>>(expression);
}

std::unique_ptr<hir::TerminalExpression> hir::Hir::get_last_terminal_expr() {
    if (!is_last_expr_terminal()) { return nullptr; }
    auto [expression] = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return std::move(std::get<std::unique_ptr<TerminalExpression>>(expression));
}

std::unique_ptr<hir::Expression> hir::Hir::get_last_expression() {
    if (m_active_expressions.empty()) { return nullptr; }
    auto expression = std::move(m_active_expressions.back());
    m_active_expressions.pop_back();
    return std::make_unique<Expression>(std::move(expression));
}

struct HirPrinter {
    static void print(const hir::Program& program) {
        for (const auto& [decl] : program.declarations) {
            std::visit([](auto&& d) -> void { HirPrinter::print(*d); }, decl);
        }
    }

    static void print(const hir::Instruction& instr) {
        std::visit([](auto&& i) -> void { HirPrinter::print(*i); }, instr.instruction);
    }

    static void print(const hir::InstructionAssignment& assignment) {
        std::cout << assignment.identifier->name << " = ";
        std::visit([](auto&& v) -> void { HirPrinter::print(*v); }, assignment.value->expression);
        std::cout << ";\n";
    }

    static void print(const hir::InstructionDeclaration& declaration) {
        if (auto* var_decl =
                std::get_if<std::unique_ptr<hir::InstructionVariableDeclaration>>(&declaration.decl
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

    static void print(const hir::InstructionFunctionDeclaration& decl) {
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
            print(instr);
        }
    }

    static void print(const hir::TerminalExpression& expr) { print(*expr.value); }

    static void print(const hir::UnaryExpression& expr) {
        std::cout << expr.op;
        print(*expr.operand);
    }

    static void print(const hir::BinaryExpression& expr) {
        print(*expr.left);
        std::cout << ' ' << expr.op << ' ';
        print(*expr.right);
    }

    static void print(const Atom& at) {
        if (const auto* ident = std::get_if<std::unique_ptr<AtomIdentifier>>(&at.atom)) {
            std::cout << (*ident)->name;
            return;
        }
        if (const auto* lit = std::get_if<std::unique_ptr<AtomIntegerLiteral>>(&at.atom)) {
            std::cout << (*lit)->value;
        }
    }
};

void hir::print_program(const Program& program) {
    HirPrinter::print(program);
}
