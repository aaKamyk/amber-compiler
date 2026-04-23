#include "ast.hpp"

#include <iostream>
#include <unordered_map>

struct ASTNodePrinter {
    void operator()(const ASTProgram& program) const {
        std::cout << "Program\n";
        for (const auto& child : program.children) {
            std::cout << "| ";
            std::visit(*this, child);
        }
    }

    void operator()(const std::unique_ptr<ASTStatement>& statement) const {
        std::cout << "- Statement\n";
        std::visit(*this, statement->statement);
    }

    void operator()(const std::unique_ptr<ASTBlockStatement>& statement) const {
        std::cout << "- Block\n";
        for (const auto& child_statement : statement->statements) {
            std::cout << "| ";
            std::visit(*this, child_statement);
        }
    }

    void operator()(const std::unique_ptr<ASTAssignmentStatement>& assignment) const {
        std::cout << "- Assignment\n";
        this->operator()(assignment->identifier);
        this->operator()(assignment->expression);
    }

    void operator()(const std::unique_ptr<ASTIfStatement>& if_statement) const {
        std::cout << "- If\n";
        this->operator()(if_statement->condition);
        this->operator()(if_statement->true_case);
        if (if_statement->has_false_case) { this->operator()(if_statement->false_case); }
    }

    void operator()(const std::unique_ptr<ASTWhileStatement>& while_statement) const {
        std::cout << "- While\n";
        this->operator()(while_statement->condition);
        this->operator()(while_statement->block);
    }

    void operator()(const std::unique_ptr<ASTReturnStatement>& return_statement) const {
        std::cout << "- Return\n";
        this->operator()(return_statement->expression);
    }
    void operator()(const std::unique_ptr<ASTDeclaration>& decl) const {
        std::cout << "- Declaration\n";
        std::visit(*this, decl->declaration);
    }

    void operator()(const std::unique_ptr<ASTFunctionDeclaration>& func) const {
        std::cout << "- Function\n";
        std::cout << "| " << func->type << "\n";
        std::cout << "| " << func->returnType << "\n";
        for (const auto& arg : func->arguments) { this->operator()(arg); }
        this->operator()(func->body);
    }

    void operator()(const std::unique_ptr<ASTFunctionArgument>& arg) const {
        std::cout << "- Function arg\n";
        this->operator()(arg->type);
        this->operator()(arg->name);
    }

    void operator()(const std::unique_ptr<ASTVariableDeclaration>& var) const {
        std::cout << "- Variable\n";
        std::cout << "| " << var->name << "\n";
        std::cout << "| " << var->type << "\n";
    }

    void operator()(const std::unique_ptr<ASTIdentifier>& ident) const {
        std::cout << "- Identifier\n";
        std::cout << "| " << ident->name << "\n";
    }

    void operator()(const std::unique_ptr<ASTIntegerLiteral>& int_literal) const {
        std::cout << "- Literal\n";
        std::cout << "| " << int_literal->value << "\n";
    }

    void operator()(const std::unique_ptr<ASTType>& type) const {
        std::cout << "- Type\n";
        std::cout << "| " << type->type << "\n";
    }

    void operator()(const std::unique_ptr<ASTExpression>& expr) const {
        std::cout << "- Expression\n";
        std::visit(*this, expr->expression);
    }

    void operator()(const std::unique_ptr<ASTBinaryExpression>& bin_expression) const {
        std::cout << "- Binary expression\n";
        std::cout << "| " << bin_expression->op << "\n";
        this->operator()(bin_expression->left);
        this->operator()(bin_expression->right);
    }

    void operator()(const std::unique_ptr<ASTUnaryExpression>& unary_expression) const {
        std::cout << "- Unary expression\n";
        std::cout << "| " << unary_expression->op << "\n";
        this->operator()(unary_expression->expression);
    }
};

std::ostream& operator<<(std::ostream& os, Type type) {
    static const std::unordered_map<Type, const char*> type_strings = {
        {Type::INT, "int"}, {Type::VOID, "void"}
    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return os << type_strings.at(type);
}

std::ostream& operator<<(std::ostream& os, const BinaryOperator op) {
    static const std::unordered_map<BinaryOperator, const char*> bin_op_strings = {
        {BinaryOperator::PLUS, "+"},
        {BinaryOperator::MINUS, "-"},
        {BinaryOperator::TIMES, "*"},
        {BinaryOperator::DIVIDE, "/"}
    };
    if (!bin_op_strings.contains(op)) { throw std::runtime_error("Unknown operator"); }
    return os << bin_op_strings.at(op);
}

std::ostream& operator<<(std::ostream& os, UnaryOperator op) {
    static const std::unordered_map<UnaryOperator, const char*> un_op_strings = {
        {UnaryOperator::MINUS, "-"}
    };
    if (!un_op_strings.contains(op)) { throw std::runtime_error("Unknown operator"); }
    return os << un_op_strings.at(op);
}