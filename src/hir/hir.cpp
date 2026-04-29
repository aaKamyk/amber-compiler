#include "hir.hpp"

Hir::Hir::Hir(ASTProgram program)
    : m_ast_program(std::move(program)) {}

Hir::HirProgram Hir::Hir::translate() {
    HirProgram program{};
    for (const auto& child : m_ast_program.children) {}
    return {};
}