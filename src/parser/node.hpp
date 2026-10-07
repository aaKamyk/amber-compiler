#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "utils/type.hpp"

namespace ast {

template <typename TypeInfo>
concept TypeMetadata = requires {
    { std::same_as<TypeInfo, type::Type> || std::same_as<TypeInfo, type::TypeID> };
};

template <TypeMetadata TypeInfo>
struct BlockStatement;
template <TypeMetadata TypeInfo>
struct AssignmentStatement;
template <TypeMetadata TypeInfo>
struct IfStatement;
template <TypeMetadata TypeInfo>
struct WhileStatement;
template <TypeMetadata TypeInfo>
struct ReturnStatement;
template <TypeMetadata TypeInfo>
struct ExpressionStatement;

template <TypeMetadata TypeInfo>
struct VariableDeclaration;
template <TypeMetadata TypeInfo>
struct FunctionParameter;
template <TypeMetadata TypeInfo>
struct FunctionDeclaration;
template <TypeMetadata TypeInfo>
struct StructField;
template <TypeMetadata TypeInfo>
struct StructDeclaration;

template <TypeMetadata TypeInfo>
class NodeVisitor {
public:
    virtual void visit(BlockStatement<TypeInfo>*) = 0;
    virtual void visit(AssignmentStatement<TypeInfo>*) = 0;
    virtual void visit(IfStatement<TypeInfo>*) = 0;
    virtual void visit(WhileStatement<TypeInfo>*) = 0;
    virtual void visit(ReturnStatement<TypeInfo>*) = 0;
    virtual void visit(ExpressionStatement<TypeInfo>*) = 0;

    virtual void visit(VariableDeclaration<TypeInfo>*) = 0;
    virtual void visit(FunctionParameter<TypeInfo>*) = 0;
    virtual void visit(FunctionDeclaration<TypeInfo>*) = 0;
    virtual void visit(StructField<TypeInfo>*) = 0;
    virtual void visit(StructDeclaration<TypeInfo>*) = 0;

    virtual ~NodeVisitor() = default;
};

template <TypeMetadata TypeInfo>
struct Node {
    virtual ~Node() = default;
    virtual void accept(NodeVisitor<TypeInfo>*) = 0;

    int line{};
    int column{};
};

template <TypeMetadata TypeInfo>
struct Statement : Node<TypeInfo> {};

template <TypeMetadata TypeInfo>
struct BlockStatement final : Statement<TypeInfo> {
    std::vector<std::unique_ptr<Node<TypeInfo>>> entries{};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    BlockStatement() = default;
    BlockStatement(std::vector<std::unique_ptr<Node<TypeInfo>>> entries)
        : entries(std::move(entries)) {}
};

template <TypeMetadata TypeInfo>
struct AssignmentStatement final : Statement<TypeInfo> {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    AssignmentStatement() = default;
};

template <TypeMetadata TypeInfo>
struct IfStatement final : Statement<TypeInfo> {
    std::unique_ptr<Expression> condition{nullptr};
    std::unique_ptr<BlockStatement<TypeInfo>> true_case{nullptr};
    std::unique_ptr<BlockStatement<TypeInfo>> false_case{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }
    [[nodiscard]] bool has_false_case() const { return false_case != nullptr; };

    IfStatement() = default;
};

template <TypeMetadata TypeInfo>
struct WhileStatement final : Statement<TypeInfo> {
    std::unique_ptr<Expression> condition{nullptr};
    std::unique_ptr<BlockStatement<TypeInfo>> block{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    WhileStatement() = default;
};

/// expression is set to nullptr if returning void;
template <TypeMetadata TypeInfo>
struct ReturnStatement final : Statement<TypeInfo> {
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    ReturnStatement() = default;
};

template <TypeMetadata TypeInfo>
struct ExpressionStatement final : Statement<TypeInfo> {
    std::unique_ptr<Expression> expression{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    ExpressionStatement() = default;
};

template <TypeMetadata TypeInfo>
struct Declaration : Node<TypeInfo> {};

template <TypeMetadata TypeInfo>
struct VariableDeclaration final : Declaration<TypeInfo> {
    TypeInfo type{type::BuiltinType::VOID};
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::unique_ptr<Expression> initializer{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }
};

template <TypeMetadata TypeInfo>
struct FunctionParameter final : Declaration<TypeInfo> {
    TypeInfo type{type::BuiltinType::VOID};
    std::unique_ptr<AtomIdentifier> name{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    FunctionParameter() = default;
    FunctionParameter(TypeInfo type, std::unique_ptr<AtomIdentifier> name)
        : type(std::move(type)), name(std::move(name)) {}
};

template <TypeMetadata TypeInfo>
struct FunctionDeclaration final : Declaration<TypeInfo> {
    TypeInfo return_type{type::BuiltinType::VOID};
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::vector<std::unique_ptr<FunctionParameter<TypeInfo>>> arguments{};
    std::unique_ptr<BlockStatement<TypeInfo>> body{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    FunctionDeclaration() = default;
    FunctionDeclaration(
        TypeInfo type, std::unique_ptr<AtomIdentifier> name,
        std::vector<std::unique_ptr<FunctionParameter<TypeInfo>>> arguments,
        std::unique_ptr<BlockStatement<TypeInfo>> body
    )
        : return_type{std::move(type)},
          name{std::move(name)},
          arguments{std::move(arguments)},
          body{std::move(body)} {}
};

template <TypeMetadata TypeInfo>
struct StructField final : Declaration<TypeInfo> {
    TypeInfo type{type::BuiltinType::VOID};
    std::unique_ptr<AtomIdentifier> name{nullptr};

    void accept(NodeVisitor<TypeInfo>* v) override { v->visit(this); }

    StructField() = default;
    StructField(TypeInfo type, std::unique_ptr<AtomIdentifier> name)
        : type{std::move(type)}, name{std::move(name)} {}
};

struct FunctionDeclaration final : Declaration {
    TypeID return_type;
    std::unique_ptr<AtomIdentifier> name{nullptr};
    std::vector<std::unique_ptr<FunctionParameter>> arguments{};
    std::unique_ptr<BlockStatement> body{nullptr};

    void accept(NodeVisitor*) override;

    FunctionDeclaration();
};

template <TypeMetadata TypeInfo>
struct Program {
    Program() = default;
    Program(Program&&) = default;
    Program& operator=(Program&&) = default;

    std::vector<std::unique_ptr<Declaration<TypeInfo>>> children{};
};

void print_program(Program<type::TypeID>& program);
}  // namespace ast