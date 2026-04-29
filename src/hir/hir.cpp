#include "hir.hpp"

#include <cassert>

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
    auto unary_expr = std::make_unique<UnaryExpression>();
}

void hir::Hir::translate(std::unique_ptr<ASTDeclaration> decl) {
    std::visit(
        [this]<typename T>(T&& d) -> void { this->translate(std::forward<T>(d)); },
        std::move(decl->declaration)
    );
}

void hir::Hir::translate(std::unique_ptr<ASTVariableDeclaration> decl) {
    const std::string_view decl_name = decl->name->name;
    if (m_symbol_table.has_symbol(decl_name)) {
        /// TODO allow for redeclaration in more nested scope
        return;
    }
    const auto id = m_symbol_table.add_symbol(decl_name.data());
    this->current_scope().add(id);
    auto var_decl =
        std::make_unique<InstructionVariableDeclaration>(decl->type->type, std::move(decl->name));
    this->append_to_program({std::move(var_decl)});
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
    this->translate(std::move(decl->body));
    this->unset_active_function();

    this->append_to_program({std::move(func_decl)});
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

void hir::Hir::append_to_program(InstructionDeclaration decl) {
    this->m_program.declarations.emplace_back(std::move(decl));
}
