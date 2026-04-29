#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "utils/atom.hpp"
#include "utils/operator.hpp"
#include "utils/type.hpp"

class SymbolTable;

struct ASTBlockStatement;
struct ASTPrimaryExpression;
struct ASTBinaryExpression;
struct ASTUnaryExpression;
struct ASTParenExpression;
struct ASTFunctionDeclaration;
struct ASTVariableDeclaration;
struct ASTStatement;
struct ASTAssignmentStatement;
struct ASTWhileStatement;
struct ASTReturnStatement;
struct ASTIfStatement;

using ASTIdentifier = AtomIdentifier;
using ASTIntegerLiteral = AtomIntegerLiteral;

struct ASTType {
    Type type;
};

struct ASTBinaryOperator {
    BinaryOperator op;
    [[nodiscard]] int get_precedence() const;
};

struct ASTUnaryOperator {
    UnaryOperator op;
    [[nodiscard]] int get_precedence() const;
};

struct ASTExpression {
    using Expression =
        std::variant<std::unique_ptr<ASTBinaryExpression>, std::unique_ptr<ASTPrimaryExpression>>;
    Expression expression;
};

struct ASTPrimaryExpression {
    using PrimaryExpression = std::variant<
        std::unique_ptr<ASTIdentifier>, std::unique_ptr<ASTIntegerLiteral>,
        std::unique_ptr<ASTParenExpression>, std::unique_ptr<ASTUnaryExpression>>;
    PrimaryExpression expression;
};

struct ASTParenExpression {
    std::unique_ptr<ASTExpression> expression;
};

struct ASTBinaryExpression {
    std::unique_ptr<ASTBinaryOperator> op;
    std::unique_ptr<ASTExpression> left;
    std::unique_ptr<ASTExpression> right;
};

struct ASTUnaryExpression {
    std::unique_ptr<ASTUnaryOperator> op;
    std::unique_ptr<ASTPrimaryExpression> expression;
};

struct ASTDeclaration {
    using Declaration = std::variant<
        std::unique_ptr<ASTFunctionDeclaration>, std::unique_ptr<ASTVariableDeclaration>>;
    Declaration declaration;
};

struct ASTVariableDeclaration {
    std::unique_ptr<ASTType> type;
    std::unique_ptr<ASTIdentifier> name;
    std::unique_ptr<ASTExpression> value;
};

struct ASTFunctionArgument {
    std::unique_ptr<ASTType> type;
    std::unique_ptr<ASTIdentifier> name;
};

struct ASTFunctionDeclaration {
    std::unique_ptr<ASTType> return_type;
    std::unique_ptr<ASTIdentifier> name;
    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments;
    std::unique_ptr<ASTBlockStatement> body;
};

struct ASTStatement {
    using Statement = std::variant<
        std::unique_ptr<ASTBlockStatement>, std::unique_ptr<ASTAssignmentStatement>,
        std::unique_ptr<ASTIfStatement>, std::unique_ptr<ASTWhileStatement>,
        std::unique_ptr<ASTReturnStatement>>;
    Statement statement;
};

struct ASTBlockStatement {
    using StatementEntry =
        std::variant<std::unique_ptr<ASTDeclaration>, std::unique_ptr<ASTStatement>>;
    std::vector<StatementEntry> entries;
};

struct ASTAssignmentStatement {
    std::unique_ptr<ASTIdentifier> identifier;
    std::unique_ptr<ASTExpression> expression;
};

struct ASTIfStatement {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTBlockStatement> true_case;
    std::unique_ptr<ASTBlockStatement> false_case;
    [[nodiscard]] bool has_false_case() const { return false_case != nullptr; };
};

struct ASTWhileStatement {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTBlockStatement> block;
};

struct ASTReturnStatement {
    std::unique_ptr<ASTExpression> expression;
};

struct ASTProgram {
    using ProgramEntry =
        std::variant<std::unique_ptr<ASTDeclaration>, std::unique_ptr<ASTStatement>>;
    std::vector<ProgramEntry> children;
};

void print_program(const ASTProgram& program);