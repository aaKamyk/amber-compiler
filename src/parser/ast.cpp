#include "ast.hpp"

#include <iostream>
#include <unordered_map>

struct ASTNodePrinter {
    static void print_program(const ASTProgram& program) {
        std::cout << "Program\n";
        const size_t size = program.children.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(program.children[i], "", is_last);
        }
    }

    template <typename Node>
    static void print_node(
        const std::unique_ptr<Node>& node, const std::string& prefix, const bool is_last
    ) {
        std::cout << prefix << (is_last ? "\\- " : "|- ");
        auto next_prefix = prefix + (is_last ? "   " : "|  ");
        ASTNodePrinter::visit_node(node, next_prefix);
    }

    template <typename... Nodes>
    static void print_node(
        const std::variant<Nodes...>& node, const std::string& prefix, const bool is_last
    ) {
        std::cout << prefix << (is_last ? "\\- " : "|- ");
        auto next_prefix = prefix + (is_last ? "   " : "|  ");
        std::visit([&](auto&& arg) { ASTNodePrinter::visit_node(arg, next_prefix); }, node);
    }

    static void visit_node(const std::unique_ptr<ASTType>& type, const std::string& prefix) {
        std::cout << "Type: " << type->type << '\n';
    }

    static void visit_node(const std::unique_ptr<ASTIdentifier>& ident, const std::string& prefix) {
        std::cout << "Identifier: " << ident->name << '\n';
    }

    static void visit_node(
        const std::unique_ptr<ASTIntegerLiteral>& int_literal, const std::string& prefix
    ) {
        std::cout << "Integer literal: " << int_literal->value << '\n';
    }

    static void visit_node(
        const std::unique_ptr<ASTBinaryOperator>& bin_op, const std::string& prefix
    ) {
        std::cout << "Binary operator: " << bin_op->op << '\n';
    }

    static void visit_node(
        const std::unique_ptr<ASTUnaryOperator>& unary_op, const std::string& prefix
    ) {
        std::cout << "Unary operator: " << unary_op->op << '\n';
    }

    static void visit_node(const std::unique_ptr<ASTExpression>& expr, const std::string& prefix) {
        std::cout << "Expression\n";

        print_node(expr->expression, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTPrimaryExpression>& expr, const std::string& prefix
    ) {
        std::cout << "Primary expression\n";

        print_node(expr->expression, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTParenExpression>& expr, const std::string& prefix
    ) {
        std::cout << "Paren expression\n";

        print_node(expr->expression, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTBinaryExpression>& bin_expression, const std::string& prefix
    ) {
        std::cout << "Binary expression\n";

        print_node(bin_expression->op, prefix, false);
        print_node(bin_expression->left, prefix, false);
        print_node(bin_expression->right, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTUnaryExpression>& unary_expression, const std::string& prefix
    ) {
        std::cout << "Unary expression\n";

        print_node(unary_expression->op, prefix, false);
        print_node(unary_expression->expression, prefix, true);
    }

    static void visit_node(const std::unique_ptr<ASTDeclaration>& decl, const std::string& prefix) {
        std::cout << "Declaration\n";

        print_node(decl->declaration, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTVariableDeclaration>& var, const std::string& prefix
    ) {
        std::cout << "Variable\n";

        print_node(var->type, prefix, false);
        print_node(var->name, prefix, var->value == nullptr);
        if (var->value) { print_node(var->value, prefix, true); }
    }

    static void visit_node(
        const std::unique_ptr<ASTFunctionArgument>& arg, const std::string& prefix
    ) {
        std::cout << "Function arg\n";

        print_node(arg->type, prefix, false);
        print_node(arg->name, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTFunctionDeclaration>& func, const std::string& prefix
    ) {
        std::cout << "Function\n";

        print_node(func->name, prefix, false);
        print_node(func->return_type, prefix, false);
        for (const auto& arg : func->arguments) { print_node(arg, prefix, false); }
        print_node(func->body, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTStatement>& statement, const std::string& prefix
    ) {
        std::cout << "Statement\n";
        print_node(statement->statement, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTBlockStatement>& statement, const std::string& prefix
    ) {
        std::cout << "Block\n";
        const size_t size = statement->statements.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(statement->statements[i], prefix, is_last);
        }
    }

    static void visit_node(
        const std::unique_ptr<ASTAssignmentStatement>& assignment, const std::string& prefix
    ) {
        std::cout << "Assignment\n";

        print_node(assignment->identifier, prefix, false);
        print_node(assignment->expression, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTIfStatement>& if_statement, const std::string& prefix
    ) {
        std::cout << "If\n";

        print_node(if_statement->condition, prefix, false);
        print_node(if_statement->true_case, prefix, !if_statement->has_false_case());
        if (!if_statement->has_false_case()) { return; }
        print_node(if_statement->false_case, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTWhileStatement>& while_statement, const std::string& prefix
    ) {
        std::cout << "While\n";

        print_node(while_statement->condition, prefix, false);
        print_node(while_statement->block, prefix, true);
    }

    static void visit_node(
        const std::unique_ptr<ASTReturnStatement>& return_statement, const std::string& prefix
    ) {
        std::cout << "Return\n";

        print_node(return_statement->expression, prefix, true);
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
    ASTNodePrinter::print_program(program);
}