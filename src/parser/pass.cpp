#include "pass.hpp"

#include <optional>

#include "utils/util.hpp"

ast::Program ast::ConstantFold::run() {
    for (auto& decl : this->m_program.children) { this->pass(decl); }
    return std::move(m_program);
}

void ast::ConstantFold::visit(BlockStatement* block_statement) {
    for (auto& entry : block_statement->entries) { this->pass(entry); }
}

void ast::ConstantFold::visit(AssignmentStatement* assignment_statement) {
    this->pass(assignment_statement->expression);
}

void ast::ConstantFold::visit(ReturnStatement* return_statement) {
    if (return_statement->expression) { this->pass(return_statement->expression); }
}

void ast::ConstantFold::visit(IfStatement* if_statement) {
    this->pass(if_statement->condition);
}

void ast::ConstantFold::visit(WhileStatement* while_statement) {
    this->pass(while_statement->condition);
}

void ast::ConstantFold::visit(ExpressionStatement* expression_statement) {
    this->pass(expression_statement->expression);
}

void ast::ConstantFold::visit(VariableDeclaration* variable_declaration) {
    if (!variable_declaration->initializer) { return; }
    this->pass(variable_declaration->initializer);
}

void ast::ConstantFold::visit(FunctionArgument* function_argument) {}

void ast::ConstantFold::visit(FunctionDeclaration* function_declaration) {
    this->pass(function_declaration->body);
}

void ast::ConstantFold::visit(AtomExpression* atom_expression) {}

void ast::ConstantFold::visit(ParenExpression* paren_expression) {
    this->pass(paren_expression->expression);
}

void ast::ConstantFold::visit(BinaryExpression* binary_expression) {
    auto try_get_literal = []<typename T>(const Expression* expr) -> std::optional<T> {
        if (const auto* atom_expr = dynamic_cast<const AtomExpression*>(expr)) {
            if (const auto* lit =
                    dynamic_cast<const AtomIntegerLiteral*>(atom_expr->atom.get())) {
                return lit->value;
            }
        }
        return std::nullopt;
    };
    auto evaluate =
        []<typename T>(const T left, const T right, const BinaryOperator op) -> T {
        /// TODO take care of overflow
        switch (op) {
            /// arithmetic operators
            case BinaryOperator::PLUS:
                return left + right;
            case BinaryOperator::MINUS:
                return left - right;
            case BinaryOperator::TIMES:
                return left * right;
            case BinaryOperator::DIVIDE:
                if (right == 0) {
                    throw std::runtime_error("Compile-time division by zero");
                }
                return left / right;
            /// logical operators
            case BinaryOperator::EQUAL:
                return left == right;
            case BinaryOperator::NOT_EQUAL:
                return left != right;
            case BinaryOperator::LESS:
                return left < right;
            case BinaryOperator::LESS_EQUAL:
                return left <= right;
            case BinaryOperator::GREATER:
                return left > right;
            case BinaryOperator::GREATER_EQUAL:
                return left >= right;
            default:
                throw std::runtime_error("Unknown operator");
        }
    };

    this->pass(binary_expression->left);
    this->pass(binary_expression->right);

    const auto left_val = try_get_literal.operator()<int>(binary_expression->left.get());
    const auto right_val =
        try_get_literal.operator()<int>(binary_expression->right.get());
    if (left_val && right_val) {
        int folded = evaluate(*left_val, *right_val, binary_expression->op);
        m_current_expression =
            std::make_unique<AtomExpression>(std::make_unique<AtomIntegerLiteral>(folded)
            );
    }
}

void ast::ConstantFold::visit(UnaryExpression* unary_expression) {}

void ast::ConstantFold::visit(CallExpression* call_expression) {
    for (auto& expr : call_expression->arguments) { this->pass(expr); }
}
