#pragma once

#include "instruction.hpp"
#include "parser/ast.hpp"
namespace Hir {

struct HirProgram {
    std::vector<Instruction> instructions;
};

class Hir {
public:
    explicit Hir(ASTProgram program);

    HirProgram translate();

private:
    ASTProgram m_ast_program;
};
}  // namespace Hir