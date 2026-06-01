#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "expression.hpp"
#include "node.hpp"
#include "utils/type.hpp"

namespace ast {

class NodeVisitor;

struct Node {
    virtual ~Node() = default;
    virtual void accept(NodeVisitor*) = 0;

    int line{};
    int column{};
};

struct Statement : Node {};

struct BlockStatement final : Statement {
    std::vector<std::unique_ptr<Node>> entries{};

    void accept(NodeVisitor*) override;
};

struct AssignmentStatement final : Statement {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor*) override;
};

struct IfStatement final : Statement {
    std::unique_ptr<Expression> condition{nullptr};
    std::unique_ptr<BlockStatement> true_case{nullptr};
    std::unique_ptr<BlockStatement> false_case{nullptr};

    void accept(NodeVisitor*) override;
    [[nodiscard]] bool has_false_case() const { return false_case != nullptr; };
};

struct WhileStatement final : Statement {
    std::unique_ptr<Expression> condition{nullptr};
    std::unique_ptr<BlockStatement> block{nullptr};

    void accept(NodeVisitor*) override;
};

/// expression is set to nullptr if returning void;
struct ReturnStatement final : Statement {
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor*) override;
};

struct ExpressionStatement final : Statement {
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor*) override;
};

struct Declaration : Node {};

struct VariableDeclaration final : Declaration {
    TypeID type;
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::unique_ptr<Expression> initializer{nullptr};

    void accept(NodeVisitor*) override;

    VariableDeclaration();
};

struct FunctionParameter {
    TypeID type;
    std::unique_ptr<AtomIdentifier> name{nullptr};

    void accept(NodeVisitor*);

    FunctionParameter();
};

struct FunctionDeclaration final : Declaration {
    TypeID return_type;
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::vector<std::unique_ptr<FunctionParameter>> arguments{};
    std::unique_ptr<BlockStatement> body{nullptr};

    void accept(NodeVisitor*) override;

    FunctionDeclaration();
};

struct Program {
    Program(Program&&) = default;
    Program() = default;
    Program& operator=(Program&&) = default;

    std::vector<std::unique_ptr<Declaration>> children{};
};

class NodeVisitor {
public:
    virtual void visit(BlockStatement*) = 0;
    virtual void visit(AssignmentStatement*) = 0;
    virtual void visit(IfStatement*) = 0;
    virtual void visit(WhileStatement*) = 0;
    virtual void visit(ReturnStatement*) = 0;
    virtual void visit(ExpressionStatement*) = 0;

    virtual void visit(VariableDeclaration*) = 0;
    virtual void visit(FunctionParameter*) = 0;
    virtual void visit(FunctionDeclaration*) = 0;

    virtual ~NodeVisitor() = default;
};

void print_program(Program& program);
}  // namespace ast