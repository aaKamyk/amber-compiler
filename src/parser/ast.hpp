#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

enum class Type : std::uint8_t {
    INT,
    VOID,
};

enum class BinaryOperator : std::uint8_t { PLUS, MINUS, TIMES, DIVIDE };
enum class UnaryOperator : std::uint8_t { MINUS };

struct ASTFunctionDeclaration;
struct ASTVariableDeclaration;
struct ASTStatement;
struct ASTBinaryExpression;
struct ASTUnaryExpression;

struct ASTIdentifier {
    std::string name;
};

struct ASTIntegerLiteral {
    int64_t value;
};

struct ASTExpression {
    using ExpressionEntry = std::variant<
        std::unique_ptr<ASTBinaryExpression>, std::unique_ptr<ASTUnaryExpression>,
        std::unique_ptr<ASTIdentifier>, std::unique_ptr<ASTIntegerLiteral>>;
    ExpressionEntry expression;
};

struct ASTBinaryExpression {
    BinaryOperator op;
    std::unique_ptr<ASTExpression> left;
    std::unique_ptr<ASTExpression> right;
};

struct ASTUnaryExpression {
    UnaryOperator op;
    std::unique_ptr<ASTExpression> expression;
};

struct ASTDeclaration {
    using Declaration = std::variant<
        std::unique_ptr<ASTFunctionDeclaration>, std::unique_ptr<ASTVariableDeclaration>>;
    Declaration declaration;
};

struct ASTBlockStatement {
    using StatementEntry =
        std::variant<std::unique_ptr<ASTDeclaration>, std::unique_ptr<ASTStatement>>;
    std::vector<StatementEntry> statements;
};

struct ASTAssignmentStatement {
    std::unique_ptr<ASTIdentifier> identifier;
    std::unique_ptr<ASTExpression> expression;
};

struct ASTIfStatement {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTBlockStatement> true_case;
    std::unique_ptr<ASTBlockStatement> false_case;
    bool has_false_case;
};

struct ASTWhileStatement {
    std::unique_ptr<ASTExpression> condition;
    std::unique_ptr<ASTBlockStatement> block;
};

struct ASTReturnStatement {
    std::unique_ptr<ASTExpression> expression;
};

struct ASTStatement {
    using Statement = std::variant<
        std::unique_ptr<ASTBlockStatement>, std::unique_ptr<ASTAssignmentStatement>,
        std::unique_ptr<ASTIfStatement>, std::unique_ptr<ASTWhileStatement>,
        std::unique_ptr<ASTReturnStatement>>;
    Statement statement;
};

struct ASTType {
    Type type;
};

struct ASTVariableDeclaration {
    Type type;
    std::string name;
};

struct ASTFunctionArgument {
    std::unique_ptr<ASTType> type;
    std::unique_ptr<ASTIdentifier> name;
};

struct ASTFunctionDeclaration {
    Type type{};
    Type returnType{};
    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments;
    std::unique_ptr<ASTBlockStatement> body;
};

struct ASTProgram {
    using ProgramEntry =
        std::variant<std::unique_ptr<ASTDeclaration>, std::unique_ptr<ASTStatement>>;
    std::vector<ProgramEntry> children;
};

std::ostream& operator<<(std::ostream& os, Type type);
std::ostream& operator<<(std::ostream& os, BinaryOperator op);
std::ostream& operator<<(std::ostream& os, UnaryOperator op);