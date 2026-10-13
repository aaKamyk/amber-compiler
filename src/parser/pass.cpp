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
    if (if_statement->has_false_case()) { this->pass(if_statement->false_case); }
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

void ast::CollectTypes::visit(
    FunctionDeclaration<type::ast::Type>* function_declaration
) {
    /// Leave function as a symbol to be resolved later
    type::Function function_type = {
        .identifier = *function_declaration->name,
    };

    type::TypeRegistry::instance().register_type(type::Type{function_type});
}

void ast::CollectTypes::visit(StructDeclaration<type::ast::Type>* struct_declaration) {
    /// Leave struct as opaque to be resolved later
    type::Struct struct_type = {
        .identifier = *struct_declaration->name,
        .fields = std::nullopt,
    };
    type::TypeRegistry::instance().register_type(type::Type{struct_type});
}

ast::Program<type::TypeID> ast::ResolveTypes::run() {
    for (auto& decl : this->m_program.children) {
        if (auto resolved_decl = this->transform<Declaration<type::TypeID>>(decl.get())) {
            this->m_resolved_program.children.emplace_back(std::move(resolved_decl));
        }
    }

    return std::move(this->m_resolved_program);
}

void ast::ResolveTypes::visit(BlockStatement<type::ast::Type>* block_statement) {
    auto resolved_entries = std::vector<std::unique_ptr<Node<type::TypeID>>>{};
    resolved_entries.reserve(block_statement->entries.size());

    for (auto& entry : block_statement->entries) {
        resolved_entries.emplace_back(this->transform<Node<type::TypeID>>(entry.get()));
    }

    this->m_result_stack.emplace_back(
        std::make_unique<BlockStatement<type::TypeID>>(std::move(resolved_entries))
    );
}

void ast::ResolveTypes::visit(
    AssignmentStatement<type::ast::Type>* assignment_statement
) {
    auto resolved_assignment = std::make_unique<AssignmentStatement<type::TypeID>>(
        std::move(assignment_statement->identifier),
        std::move(assignment_statement->expression)
    );

    this->m_result_stack.emplace_back(std::move(resolved_assignment));
}

void ast::ResolveTypes::visit(ReturnStatement<type::ast::Type>* return_statement) {
    auto resolved_return = std::make_unique<ReturnStatement<type::TypeID>>(
        std::move(return_statement->expression)
    );

    this->m_result_stack.emplace_back(std::move(resolved_return));
}

void ast::ResolveTypes::visit(IfStatement<type::ast::Type>* if_statement) {
    auto resolved_if = std::make_unique<IfStatement<type::TypeID>>(
        std::move(if_statement->condition),
        this->transform<BlockStatement<type::TypeID>>(if_statement->true_case.get()),
        this->transform<BlockStatement<type::TypeID>>(if_statement->false_case.get())
    );

    this->m_result_stack.emplace_back(std::move(resolved_if));
}

void ast::ResolveTypes::visit(WhileStatement<type::ast::Type>* while_statement) {
    auto resolved_while = std::make_unique<WhileStatement<type::TypeID>>(
        std::move(while_statement->condition),
        this->transform<BlockStatement<type::TypeID>>(while_statement->block.get())
    );

    this->m_result_stack.emplace_back(std::move(resolved_while));
}

void ast::ResolveTypes::visit(
    ExpressionStatement<type::ast::Type>* expression_statement
) {
    auto resolved_expression = std::make_unique<ExpressionStatement<type::TypeID>>(
        std::move(expression_statement->expression)
    );

    this->m_result_stack.emplace_back(std::move(resolved_expression));
}

void ast::ResolveTypes::visit(
    VariableDeclaration<type::ast::Type>* variable_declaration
) {
    auto resolved_variable = std::make_unique<VariableDeclaration<type::TypeID>>(
        ResolveTypes::resolve_type(variable_declaration->type),
        std::move(variable_declaration->name),
        std::move(variable_declaration->initializer)
    );

    this->m_result_stack.emplace_back(std::move(resolved_variable));
}

void ast::ResolveTypes::visit(FunctionParameter<type::ast::Type>* function_argument) {
    auto resolved_argument = std::make_unique<FunctionParameter<type::TypeID>>(
        ResolveTypes::resolve_type(function_argument->type),
        std::move(function_argument->name)
    );

    this->m_result_stack.emplace_back(std::move(resolved_argument));
}

void ast::ResolveTypes::visit(
    FunctionDeclaration<type::ast::Type>* function_declaration
) {
    auto return_type_id = resolve_type(function_declaration->return_type);

    // Transform parameters
    auto resolved_args = std::vector<std::unique_ptr<FunctionParameter<type::TypeID>>>{};
    resolved_args.reserve(function_declaration->arguments.size());
    for (auto& arg : function_declaration->arguments) {
        resolved_args.emplace_back(
            this->transform<FunctionParameter<type::TypeID>>(arg.get())
        );
    }

    // Transform body
    auto resolved_body =
        this->transform<BlockStatement<type::TypeID>>(function_declaration->body.get());

    /// Obtain current registered function from type registry
    const auto id =
        type::TypeRegistry::instance().get_type_id(function_declaration->name->name);
    auto function_type =
        type::TypeRegistry::instance().get_type(id).get<type::Function>();

    /// Fill out the fields in the new type
    function_type.return_type = return_type_id;
    function_type.parameters.reserve(resolved_args.size());
    for (const auto& resolved_arg : resolved_args) {
        function_type.parameters.emplace_back(resolved_arg->type);
    }

    /// Update the registry function entry
    type::TypeRegistry::instance().update_type(id, std::move(function_type));

    this->m_result_stack.emplace_back(
        std::make_unique<FunctionDeclaration<type::TypeID>>(
            return_type_id, std::move(function_declaration->name),
            std::move(resolved_args), std::move(resolved_body)
        )
    );
}

void ast::ResolveTypes::visit(StructField<type::ast::Type>* struct_field) {
    auto resolved_field = std::make_unique<StructField<type::TypeID>>(
        ResolveTypes::resolve_type(struct_field->type), std::move(struct_field->name)
    );

    this->m_result_stack.emplace_back(std::move(resolved_field));
}

void ast::ResolveTypes::visit(StructDeclaration<type::ast::Type>* struct_declaration) {
    auto resolved_fields = std::vector<std::unique_ptr<StructField<type::TypeID>>>();
    resolved_fields.reserve(struct_declaration->fields.size());

    for (auto& field : struct_declaration->fields) {
        resolved_fields.emplace_back(
            this->transform<StructField<type::TypeID>>(field.get())
        );
    }

    /// Obrain current registered struct from type registry
    const auto id =
        type::TypeRegistry::instance().get_type_id(struct_declaration->name->name);
    auto struct_type = type::TypeRegistry::instance().get_type(id).get<type::Struct>();

    /// Fill out all the fields in the new type
    struct_type.fields = std::vector<type::StructField>{};
    struct_type.fields->reserve(resolved_fields.size());
    for (const auto& resolved_field : resolved_fields) {
        struct_type.fields->emplace_back(*resolved_field->name, resolved_field->type);
    }

    /// Update the type registry struct entry
    type::TypeRegistry::instance().update_type(id, std::move(struct_type));

    this->m_result_stack.emplace_back(
        std::make_unique<StructDeclaration<type::TypeID>>(
            std::move(struct_declaration->name), std::move(resolved_fields)
        )
    );
}

void ast::ResolveTypes::visit(AtomExpression* atom_expression) {}

void ast::ResolveTypes::visit(ParenExpression* paren_expression) {}

void ast::ResolveTypes::visit(BinaryExpression* binary_expression) {}

void ast::ResolveTypes::visit(UnaryExpression* unary_expression) {}

void ast::ResolveTypes::visit(CallExpression* call_expression) {}

type::TypeID ast::ResolveTypes::resolve_type(const type::ast::Type& type) {
    return type::TypeRegistry::instance().get_type_id(type);
}
