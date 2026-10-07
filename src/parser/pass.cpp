#include "pass.hpp"

#include <optional>

#include "parser/node.hpp"

ast::Program<type::TypeID> ast::ConstantFold::run() {
    for (auto& decl : this->m_program.children) { this->pass(decl); }
    return std::move(m_program);
}

void ast::ConstantFold::visit(BlockStatement<type::TypeID>* block_statement) {
    for (auto& entry : block_statement->entries) { this->pass(entry); }
}

void ast::ConstantFold::visit(AssignmentStatement<type::TypeID>* assignment_statement) {
    this->pass(assignment_statement->expression);
}

void ast::ConstantFold::visit(ReturnStatement<type::TypeID>* return_statement) {
    if (return_statement->expression) { this->pass(return_statement->expression); }
}

void ast::ConstantFold::visit(IfStatement<type::TypeID>* if_statement) {
    this->pass(if_statement->condition);

    this->pass(if_statement->true_case);
    this->pass(if_statement->false_case);
}

void ast::ConstantFold::visit(WhileStatement<type::TypeID>* while_statement) {
    this->pass(while_statement->condition);
}

void ast::ConstantFold::visit(ExpressionStatement<type::TypeID>* expression_statement) {
    this->pass(expression_statement->expression);
}
void ast::ConstantFold::visit(VariableDeclaration<type::TypeID>* variable_declaration) {
    if (!variable_declaration->initializer) { return; }
    this->pass(variable_declaration->initializer);
}

void ast::ConstantFold::visit(FunctionParameter<type::TypeID>* function_argument) {}

void ast::ConstantFold::visit(FunctionDeclaration<type::TypeID>* function_declaration) {
    this->pass(function_declaration->body);
}

void ast::ConstantFold::visit(StructField<type::TypeID>* struct_field) {}

void ast::ConstantFold::visit(StructDeclaration<type::TypeID>* struct_declaration) {}

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
                    throw std::logic_error("Compile-time division by zero");
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
        m_current_expression = std::make_unique<AtomExpression>(
            std::make_unique<AtomIntegerLiteral>(folded)
        );
    }
}

void ast::ConstantFold::visit(UnaryExpression* unary_expression) {}

void ast::ConstantFold::visit(CallExpression* call_expression) {
    for (auto& expr : call_expression->arguments) { this->pass(expr); }
}

ast::Program<type::ast::Type> ast::CollectTypes::run() {
    for (auto& decl : this->m_program.children) { this->pass(decl); }
    return std::move(this->m_program);
}

void ast::CollectTypes::visit(BlockStatement<type::ast::Type>* block_statement) {
    for (const auto& entry : block_statement->entries) { entry->accept(this); }
}

void ast::CollectTypes::visit(StructDeclaration<type::ast::Type>* struct_declaration) {
    type::Struct struct_type = {
        .identifier = *struct_declaration->name,
        .fields = std::nullopt,
    };
    type::TypeRegistry::instance().register_type(type::Type{struct_type});
}

ast::Program<type::TypeID> ast::ResolveTypes::run() {
    for (auto& decl : this->m_program.children) {
        this->pass(decl);
        if (this->m_current_node) {
            this->m_resolved_program.children.emplace_back(
                std::unique_ptr<Declaration<type::TypeID>>{
                    dynamic_cast<Declaration<type::TypeID>*>(
                        this->m_current_node.release()
                    )
                }
            );
        }
    }

    return std::move(this->m_resolved_program);
}

void ast::ResolveTypes::visit(BlockStatement<type::ast::Type>* block_statement) {
    for (auto& node : block_statement->entries) { this->pass(node); }
}

void ast::ResolveTypes::visit(
    AssignmentStatement<type::ast::Type>* assignment_statement
) {}

void ast::ResolveTypes::visit(ReturnStatement<type::ast::Type>* return_statement) {}

void ast::ResolveTypes::visit(IfStatement<type::ast::Type>* if_statement) {}

void ast::ResolveTypes::visit(WhileStatement<type::ast::Type>* while_statement) {}

void ast::ResolveTypes::visit(
    ExpressionStatement<type::ast::Type>* expression_statement
) {}

void ast::ResolveTypes::visit(
    VariableDeclaration<type::ast::Type>* variable_declaration
) {}

void ast::ResolveTypes::visit(FunctionParameter<type::ast::Type>* function_argument) {
    auto type_id = type::TypeRegistry::instance().get_type_id(function_argument->type);

    auto* function_declaration =
        dynamic_cast<FunctionDeclaration<type::TypeID>*>(this->m_current_node.get());
    function_declaration->arguments.emplace_back(
        std::make_unique<FunctionParameter<type::TypeID>>(
            type_id, std::move(function_argument->name)
        )
    );
}

void ast::ResolveTypes::visit(
    FunctionDeclaration<type::ast::Type>* function_declaration
) {
    auto return_type_id =
        type::TypeRegistry::instance().get_type_id(function_declaration->return_type);

    auto arguments = std::vector<std::unique_ptr<FunctionParameter<type::TypeID>>>{};
    arguments.reserve(function_declaration->arguments.size());
    auto block_entries = std::vector<std::unique_ptr<Node<type::TypeID>>>{};
    block_entries.reserve(function_declaration->body->entries.size());

    auto resolved_function_declaration =
        std::make_unique<FunctionDeclaration<type::TypeID>>(
            return_type_id, std::move(function_declaration->name), std::move(arguments),
            std::make_unique<BlockStatement<type::TypeID>>(std::move(block_entries))
        );

    // Process function arguments
    this->m_current_node = std::move(resolved_function_declaration);
    for (auto& arg : function_declaration->arguments) { this->pass(arg); }
    resolved_function_declaration = std::unique_ptr<FunctionDeclaration<type::TypeID>>{
        dynamic_cast<FunctionDeclaration<type::TypeID>*>(this->m_current_node.release())
    };

    // Process code block
    this->m_current_node = std::move(resolved_function_declaration->body);
    this->pass(function_declaration->body);
    resolved_function_declaration->body = std::unique_ptr<BlockStatement<type::TypeID>>{
        dynamic_cast<BlockStatement<type::TypeID>*>(this->m_current_node.release())
    };
    this->m_current_node = std::move(resolved_function_declaration);
}

void ast::ResolveTypes::visit(StructField<type::ast::Type>* struct_field) {
    const auto type_id = type::TypeRegistry::instance().get_type_id(struct_field->type);

    auto* struct_declaration =
        dynamic_cast<StructDeclaration<type::TypeID>*>(this->m_current_node.get());
    struct_declaration->fields.emplace_back(
        std::make_unique<StructField<type::TypeID>>(
            type_id, std::move(struct_field->name)
        )
    );
}

void ast::ResolveTypes::visit(StructDeclaration<type::ast::Type>* struct_declaration) {
    auto fields = std::vector<std::unique_ptr<StructField<type::TypeID>>>();
    fields.reserve(struct_declaration->fields.size());
    this->m_current_node = std::make_unique<StructDeclaration<type::TypeID>>(
        std::move(struct_declaration->name), std::move(fields)
    );

    for (auto& field : struct_declaration->fields) { this->pass(field); }
}

void ast::ResolveTypes::visit(AtomExpression* atom_expression) {}

void ast::ResolveTypes::visit(ParenExpression* paren_expression) {}

void ast::ResolveTypes::visit(BinaryExpression* binary_expression) {}

void ast::ResolveTypes::visit(UnaryExpression* unary_expression) {}

void ast::ResolveTypes::visit(CallExpression* call_expression) {}
