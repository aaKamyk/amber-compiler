#include "ast.hpp"

#include <iostream>
#include <unordered_map>

struct ASTNodePrinter {
    int depth{};
    void print_indent() const {
        for (int i = 0; i < depth; i++) { std::cout << "  "; }
    }

    void operator()(const ASTProgram& program) {
        print_indent();
        std::cout << "Program\n";

        depth++;
        for (const auto& child : program.children) { std::visit(*this, child); }
        depth--;
    }

    void operator()(const std::unique_ptr<ASTStatement>& statement) {
        print_indent();
        std::cout << "- Statement\n";

        depth++;
        std::visit(*this, statement->statement);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTBlockStatement>& statement) {
        print_indent();
        std::cout << "- Block\n";

        depth++;
        for (const auto& child_statement : statement->statements) {
            std::visit(*this, child_statement);
        }
        depth--;
    }

    void operator()(const std::unique_ptr<ASTAssignmentStatement>& assignment) {
        print_indent();
        std::cout << "- Assignment\n";

        depth++;
        this->operator()(assignment->identifier);
        this->operator()(assignment->expression);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTIfStatement>& if_statement) {
        print_indent();
        std::cout << "- If\n";

        depth++;
        this->operator()(if_statement->condition);
        this->operator()(if_statement->true_case);
        if (if_statement->has_false_case()) { this->operator()(if_statement->false_case); }
        depth--;
    }

    void operator()(const std::unique_ptr<ASTWhileStatement>& while_statement) {
        print_indent();
        std::cout << "- While\n";

        depth++;
        this->operator()(while_statement->condition);
        this->operator()(while_statement->block);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTReturnStatement>& return_statement) {
        print_indent();
        std::cout << "- Return\n";

        depth++;
        this->operator()(return_statement->expression);
        depth--;
    }
    void operator()(const std::unique_ptr<ASTDeclaration>& decl) {
        print_indent();
        std::cout << "- Declaration\n";

        depth++;
        std::visit(*this, decl->declaration);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTFunctionDeclaration>& func) {
        print_indent();
        std::cout << "- Function\n";

        depth++;
        this->operator()(func->name);
        this->operator()(func->return_type);
        for (const auto& arg : func->arguments) { this->operator()(arg); }
        this->operator()(func->body);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTFunctionArgument>& arg) {
        print_indent();
        std::cout << "- Function arg\n";

        depth++;
        this->operator()(arg->type);
        this->operator()(arg->name);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTVariableDeclaration>& var) {
        print_indent();
        std::cout << "- Variable\n";

        depth++;
        this->operator()(var->type);
        this->operator()(var->name);
        if (var->value) { this->operator()(var->value); }
        depth--;
    }

    void operator()(const std::unique_ptr<ASTIdentifier>& ident) {
        print_indent();
        std::cout << "- Identifier\n";
        depth++;
        print_indent();
        std::cout << "| " << ident->name << "\n";
        depth--;
    }

    void operator()(const std::unique_ptr<ASTIntegerLiteral>& int_literal) {
        print_indent();
        std::cout << "- Literal\n";

        depth++;
        print_indent();
        std::cout << "| " << int_literal->value << "\n";
        depth--;
    }

    void operator()(const std::unique_ptr<ASTType>& type) {
        print_indent();
        std::cout << "- Type\n";

        depth++;
        print_indent();
        std::cout << "| " << type->type << "\n";
        depth--;
    }

    void operator()(const std::unique_ptr<ASTExpression>& expr) {
        print_indent();
        std::cout << "- Expression\n";

        depth++;
        std::visit(*this, expr->expression);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTPrimaryExpression>& expr) {
        print_indent();
        std::cout << "- Primary expression\n";

        depth++;
        std::visit(*this, expr->expression);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTParenExpression>& expr) {
        print_indent();
        std::cout << "- Paren expression\n";

        depth++;
        this->operator()(expr->expression);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTBinaryExpression>& bin_expression) {
        print_indent();
        std::cout << "- Binary expression\n";

        depth++;
        print_indent();
        std::cout << "| " << bin_expression->op << "\n";
        this->operator()(bin_expression->left);
        this->operator()(bin_expression->right);
        depth--;
    }

    void operator()(const std::unique_ptr<ASTUnaryExpression>& unary_expression) {
        print_indent();
        std::cout << "- Unary expression\n";

        depth++;
        print_indent();
        std::cout << "| " << unary_expression->op << "\n";
        this->operator()(unary_expression->expression);
        depth--;
    }
};

int ASTBinaryOperator::get_precedence() const {
    static const std::unordered_map<BinaryOperator, int> precedences = {
        {BinaryOperator::PLUS, 1},
        {BinaryOperator::MINUS, 1},
        {BinaryOperator::TIMES, 2},
        {BinaryOperator::DIVIDE, 2},
    };
    return precedences.at(this->op);
}

int ASTUnaryOperator::get_precedence() const {
    static const std::unordered_map<UnaryOperator, int> precedences = {{UnaryOperator::MINUS, 1}};
    return precedences.at(this->op);
}

std::ostream& operator<<(std::ostream& os, const Type type) {
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

std::ostream& operator<<(std::ostream& os, const UnaryOperator op) {
    static const std::unordered_map<UnaryOperator, const char*> un_op_strings = {
        {UnaryOperator::MINUS, "-"}
    };
    if (!un_op_strings.contains(op)) { throw std::runtime_error("Unknown operator"); }
    return os << un_op_strings.at(op);
}

void print_program(const ASTProgram& program) {
    ASTNodePrinter printer{};
    printer(program);
}
