#pragma once

#include <map>
#include <optional>
#include <vector>

#include "lexer/token.hpp"
#include "node.hpp"
#include "utils/context.hpp"

namespace ast {
class Parser;

template <typename Node>
struct NodeParser {
    static std::unique_ptr<Node> parse(Parser& ctx) { return nullptr; }
};

template <>
struct NodeParser<Expression> {
    static std::unique_ptr<Expression> parse(Parser& ctx, int min_precedence = 0);
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, CompilationContext& context)
        : m_tokens(std::move(tokens)), m_ctx(context) {}

    Program parse();
    [[nodiscard]] bool was_successful() const { return m_successfully_parsed; }
    [[nodiscard]] const std::vector<SyntaxError>& get_errors() const {
        return m_ctx.syntax_errors();
    };

private:
    template <typename T>
    friend struct NodeParser;

    void consume(size_t n = 1);
    bool expect(TokenType type);
    template <typename... Types>
        requires(std::is_same_v<Types, TokenType> && ...)
    bool expect(Types... types) {
        const size_t start = save();
        if (!(expect(types) && ...)) {
            restore(start);
            return false;
        }
        return true;
    }
    [[nodiscard]] bool match(TokenType type, size_t offset = 0) const;
    template <typename... Types>
        requires(std::is_same_v<Types, TokenType> && ...)
    [[nodiscard]] bool match(Types... types) const {
        size_t index{0};
        return (match(types, index++) && ...);
    }
    template <typename... Types>
        requires(std::is_same_v<Types, TokenType> && ...)
    [[nodiscard]] bool match_any(Types... types) const {
        return (match(types) || ...);
    }

    void restore(size_t index);
    [[nodiscard]] size_t save() const { return m_current_token; }
    [[nodiscard]] Token peek(size_t offset = 0) const;
    [[nodiscard]] Token current_token() const;

    void add_error(const std::string& message);

    std::vector<Token> m_tokens;
    size_t m_current_token{};
    bool m_successfully_parsed{true};
    CompilationContext& m_ctx;

    template <typename T>

    std::unique_ptr<T> try_parse() {
        const auto start = save();
        const auto current_token = this->current_token();
        if (auto parse_result = NodeParser<T>::parse(*this)) {
            if constexpr (std::is_base_of_v<Node, T>) {
                parse_result->line = current_token.line;
                parse_result->column = current_token.column;
            }
            while (!m_ctx.syntax_errors().empty() &&
                   m_ctx.syntax_errors().back().token_num < m_current_token) {
                m_ctx.syntax_errors().pop_back();
            }
            return parse_result;
        }
        restore(start);
        return nullptr;
    }

    template <typename T>
        requires std::same_as<T, BinaryOperator> || std::same_as<T, UnaryOperator> ||
                 std::same_as<T, Type>
    std::optional<T> try_parse() {
        const auto start = save();
        if (auto parse_result = NodeParser<T>::parse(*this)) {
            if (!m_ctx.syntax_errors().empty() &&
                m_ctx.syntax_errors().back().token_num < m_current_token) {
                m_ctx.syntax_errors().pop_back();
            }
            return parse_result;
        }
        restore(start);
        return std::nullopt;
    }

    std::unique_ptr<Expression> try_parse(const int precedence = 0) {
        const auto start = save();
        if (auto expr = NodeParser<Expression>::parse(*this, precedence)) {
            if (!m_ctx.syntax_errors().empty() &&
                m_ctx.syntax_errors().back().token_num < m_current_token) {
                m_ctx.syntax_errors().pop_back();
            }
            return expr;
        }
        restore(start);
        return nullptr;
    }

    template <typename T>
    bool parse_into(Parser& ctx, T& destination) {
        auto res = ctx.try_parse<T>();
        if (!res) { return false; }
        destination = *res;
        return true;
    }

    template <typename T>
    bool parse_into(Parser& ctx, std::unique_ptr<T>& destination) {
        auto res = ctx.try_parse<T>();
        if (!res) { return false; }
        destination = std::move(res);
        return true;
    }
};

Program parse(std::vector<Token> tokens, CompilationContext& ctx);
}  // namespace ast

template <>
struct ast::NodeParser<Type> {
    static std::optional<Type> parse(Parser& ctx) {
        /// TODO make this expandable
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::INT:
                ctx.consume(1);
                return Type::INT;
            case TokenType::VOID:
                ctx.consume(1);
                return Type::VOID;
            default:
                const auto next_token = ctx.peek(1);
                ctx.add_error("Expected type before \'" + next_token.to_string() + '\'');
                return std::nullopt;
        }
    }
};

template <>
struct ast::NodeParser<AtomIdentifier> {
    static std::unique_ptr<AtomIdentifier> parse(Parser& ctx) {
        auto token = ctx.peek();
        if (!ctx.expect(TokenType::IDENTIFIER)) {
            const auto next_token = ctx.peek(1);
            ctx.add_error(
                "Expected identifier before \'" + next_token.to_string() + '\''
            );
            return nullptr;
        }
        return std::make_unique<AtomIdentifier>(
            std::move(std::get<std::string>(token.data))
        );
    }
};

template <>
struct ast::NodeParser<AtomIntegerLiteral> {
    static std::unique_ptr<AtomIntegerLiteral> parse(Parser& ctx) {
        const auto token = ctx.peek();
        if (!ctx.expect(TokenType::INTEGER_LITERAL)) { return nullptr; }
        try {
            return std::make_unique<AtomIntegerLiteral>(
                std::stol(std::get<std::string>(token.data))
            );
        } catch (const std::out_of_range& _) {
            ctx.add_error("Invalid integer literal: \'" + token.to_string() + '\'');
            return nullptr;
        }
    }
};

template <>
struct ast::NodeParser<ast::BinaryOperator> {
    static std::optional<BinaryOperator> parse(Parser& ctx) {
        static const std::map<TokenType, BinaryOperator> mapping = {
            {TokenType::PLUS, BinaryOperator::PLUS},
            {TokenType::MINUS, BinaryOperator::MINUS},
            {TokenType::ASTERISK, BinaryOperator::TIMES},
            {TokenType::DIVIDE, BinaryOperator::DIVIDE},
            {TokenType::EQUALS_EQUALS, BinaryOperator::EQUAL},
            {TokenType::NOT_EQUALS, BinaryOperator::NOT_EQUAL},
            {TokenType::LESS_THAN, BinaryOperator::LESS},
            {TokenType::LESS_EQUAL_THAN, BinaryOperator::LESS_EQUAL},
            {TokenType::GREATER_THAN, BinaryOperator::GREATER},
            {TokenType::GREATER_EQUAL_THAN, BinaryOperator::GREATER_EQUAL}
        };
        const auto token = ctx.peek();
        if (const auto it = mapping.find(token.type); it != mapping.end()) {
            ctx.consume(1);
            return it->second;
        }
        return std::nullopt;
    }
};

template <>
struct ast::NodeParser<ast::UnaryOperator> {
    static std::optional<UnaryOperator> parse(Parser& ctx) {
        static const std::map<TokenType, UnaryOperator> mapping = {
            {TokenType::MINUS, UnaryOperator::MINUS},
            {TokenType::LOGICAL_NOT, UnaryOperator::NOT}
        };
        const auto token = ctx.peek();
        if (const auto it = mapping.find(token.type); it != mapping.end()) {
            ctx.consume(1);
            return it->second;
        }
        return std::nullopt;
    }
};

template <>
struct ast::NodeParser<ast::PrimaryExpression> {
    static std::unique_ptr<PrimaryExpression> parse(Parser& ctx) {
        if (ctx.match(TokenType::IDENTIFIER)) {
            if (auto call_expr = ctx.try_parse<CallExpression>()) { return call_expr; }
        }
        if (auto atom_expr = ctx.try_parse<AtomExpression>()) { return atom_expr; }
        if (ctx.match(TokenType::PAREN_START)) {
            if (auto paren_expr = ctx.try_parse<ParenExpression>()) { return paren_expr; }
            return nullptr;
        }
        if (auto unary_expr = ctx.try_parse<UnaryExpression>()) { return unary_expr; }
        ctx.add_error("Expected primary expression");
        return nullptr;
    }
};

template <>
struct ast::NodeParser<ast::AtomExpression> {
    static std::unique_ptr<AtomExpression> parse(Parser& ctx) {
        if (auto ident = ctx.try_parse<AtomIdentifier>()) {
            return std::make_unique<AtomExpression>(std::move(ident));
        }
        if (auto int_literal = ctx.try_parse<AtomIntegerLiteral>()) {
            return std::make_unique<AtomExpression>(std::move(int_literal));
        }
        return nullptr;
    }
};

template <>
struct ast::NodeParser<ast::ParenExpression> {
    static std::unique_ptr<ParenExpression> parse(Parser& ctx) {
        auto paren_expr = std::make_unique<ParenExpression>();
        if (!ctx.expect(TokenType::PAREN_START)) {
            ctx.add_error("Expected opening parenthesis");
            return nullptr;
        }
        if (!ctx.parse_into<Expression>(ctx, paren_expr->expression)) { return nullptr; }
        if (!ctx.expect(TokenType::PAREN_END)) {
            ctx.add_error("Expected closing parenthesis");
            return nullptr;
        }
        return paren_expr;
    }
};

template <>
struct ast::NodeParser<ast::UnaryExpression> {
    static std::unique_ptr<UnaryExpression> parse(Parser& ctx) {
        auto unary_expr = std::make_unique<UnaryExpression>();
        if (!ctx.parse_into<UnaryOperator>(ctx, unary_expr->op)) { return nullptr; }
        if (!ctx.parse_into<PrimaryExpression>(ctx, unary_expr->expression)) {
            return nullptr;
        }
        return unary_expr;
    }
};

template <>
struct ast::NodeParser<ast::CallExpression> {
    static std::unique_ptr<CallExpression> parse(Parser& ctx) {
        auto call_expr = std::make_unique<CallExpression>();
        if (!ctx.parse_into<AtomIdentifier>(ctx, call_expr->function)) { return nullptr; }
        if (!ctx.expect(TokenType::PAREN_START)) { return nullptr; }
        while (auto arg = ctx.try_parse<Expression>()) {
            call_expr->arguments.push_back(std::move(arg));
            if (!ctx.expect(TokenType::COMMA)) { break; }
            /// closing paren after a comma - invalid
            if (ctx.match(TokenType::PAREN_END)) {
                ctx.add_error("Misplaced comma");
                return nullptr;
            }
        }
        if (!ctx.expect(TokenType::PAREN_END)) { return nullptr; }
        return call_expr;
    }
};

inline std::unique_ptr<ast::Expression> ast::NodeParser<ast::Expression>::parse(
    Parser& ctx, const int min_precedence
) {
    std::unique_ptr<Expression> result;
    if (result = ctx.try_parse<PrimaryExpression>(); !result) { return nullptr; }

    while (true) {
        const auto start = ctx.save();
        auto bin_op = ctx.try_parse<BinaryOperator>();
        if (!bin_op || get_precedence(*bin_op) <= min_precedence) {
            ctx.restore(start);
            break;
        }
        auto right = ctx.try_parse(get_precedence(*bin_op));
        if (!right) { return nullptr; }
        // result becomes the left side of the binary expression
        auto bin_expr = std::make_unique<BinaryExpression>(
            *bin_op, std::move(result), std::move(right)
        );
        result = std::move(bin_expr);
    }
    return result;
}

template <>
struct ast::NodeParser<ast::BlockStatement> {
    static std::unique_ptr<BlockStatement> parse(Parser& ctx) {
        auto block_statement = std::make_unique<BlockStatement>();
        if (!ctx.expect(TokenType::CURLY_BRACE_START)) {
            const auto next_token = ctx.peek();
            ctx.add_error(
                "expected opening brace before \'" + next_token.to_string() + "\'"
            );
            return nullptr;
        }
        while (true) {
            if (auto decl = ctx.try_parse<Declaration>()) {
                block_statement->entries.emplace_back(std::move(decl));
            } else if (auto statement = ctx.try_parse<Statement>()) {
                block_statement->entries.emplace_back(std::move(statement));
            } else {
                break;
            }
        }
        if (!ctx.expect(TokenType::CURLY_BRACE_END)) { return nullptr; }
        return block_statement;
    }
};

template <>
struct ast::NodeParser<ast::AssignmentStatement> {
    static std::unique_ptr<AssignmentStatement> parse(Parser& ctx) {
        auto assignment_statement = std::make_unique<AssignmentStatement>();
        if (!ctx.parse_into<AtomIdentifier>(ctx, assignment_statement->identifier)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::EQUALS)) { return nullptr; }
        if (!ctx.parse_into<Expression>(ctx, assignment_statement->expression)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::SEMICOLON)) { return nullptr; }
        return assignment_statement;
    }
};

template <>
struct ast::NodeParser<ast::IfStatement> {
    static std::unique_ptr<IfStatement> parse(Parser& ctx) {
        auto if_statement = std::make_unique<IfStatement>();
        if (!ctx.expect(TokenType::IF, TokenType::PAREN_START)) { return nullptr; }

        if (!ctx.parse_into<Expression>(ctx, if_statement->condition)) { return nullptr; }
        if (!ctx.expect(TokenType::PAREN_END)) { return nullptr; }
        if (!ctx.parse_into<BlockStatement>(ctx, if_statement->true_case)) {
            return nullptr;
        }
        if (ctx.expect(TokenType::ELSE)) {
            if (!ctx.parse_into<BlockStatement>(ctx, if_statement->false_case)) {
                return nullptr;
            }
            return if_statement;
        }
        if_statement->false_case = nullptr;
        return if_statement;
    }
};

template <>
struct ast::NodeParser<ast::WhileStatement> {
    static std::unique_ptr<WhileStatement> parse(Parser& ctx) {
        auto while_statement = std::make_unique<WhileStatement>();
        if (!ctx.expect(TokenType::WHILE, TokenType::PAREN_START)) { return nullptr; }
        if (!ctx.parse_into<Expression>(ctx, while_statement->condition)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::PAREN_END)) { return nullptr; }
        if (!ctx.parse_into<BlockStatement>(ctx, while_statement->block)) {
            return nullptr;
        }
        return while_statement;
    }
};

template <>
struct ast::NodeParser<ast::ReturnStatement> {
    static std::unique_ptr<ReturnStatement> parse(Parser& ctx) {
        auto return_statement = std::make_unique<ReturnStatement>();
        if (!ctx.expect(TokenType::RETURN)) { return nullptr; }
        if (!ctx.parse_into<Expression>(ctx, return_statement->expression)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::SEMICOLON)) { return nullptr; }
        return return_statement;
    }
};

template <>
struct ast::NodeParser<ast::ExpressionStatement> {
    static std::unique_ptr<ExpressionStatement> parse(Parser& ctx) {
        auto expression_statement = std::make_unique<ExpressionStatement>();
        if (!ctx.parse_into<Expression>(ctx, expression_statement->expression)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::SEMICOLON)) { return nullptr; }
        return expression_statement;
    }
};

template <>
struct ast::NodeParser<ast::VariableDeclaration> {
    static std::unique_ptr<VariableDeclaration> parse(Parser& ctx) {
        auto variable_declaration = std::make_unique<VariableDeclaration>();
        if (!ctx.parse_into<Type>(ctx, variable_declaration->type)) { return nullptr; }
        if (!ctx.parse_into<AtomIdentifier>(ctx, variable_declaration->name)) {
            return nullptr;
        }
        if (ctx.expect(TokenType::SEMICOLON)) {
            variable_declaration->value = nullptr;
            return variable_declaration;
        }
        if (ctx.expect(TokenType::EQUALS)) {
            if (!ctx.parse_into<Expression>(ctx, variable_declaration->value)) {
                return nullptr;
            }
            if (!ctx.expect(TokenType::SEMICOLON)) {
                const auto next_token = ctx.peek();
                ctx.add_error("Expected \':\' before " + next_token.to_string());
                return nullptr;
            }
            return variable_declaration;
        }
        return nullptr;
    }
};

template <>
struct ast::NodeParser<ast::FunctionArgument> {
    static std::unique_ptr<FunctionArgument> parse(Parser& ctx) {
        auto arg = std::make_unique<FunctionArgument>();
        if (!ctx.parse_into<Type>(ctx, arg->type)) { return nullptr; }
        if (!ctx.parse_into<AtomIdentifier>(ctx, arg->name)) { return nullptr; }
        return arg;
    }
};

template <>
struct ast::NodeParser<ast::FunctionDeclaration> {
    static std::unique_ptr<FunctionDeclaration> parse(Parser& ctx) {
        auto func_declaration = std::make_unique<FunctionDeclaration>();
        if (!ctx.expect(TokenType::FN)) { return nullptr; }
        if (!ctx.parse_into<AtomIdentifier>(ctx, func_declaration->name)) {
            return nullptr;
        }
        if (!ctx.expect(TokenType::PAREN_START)) { return nullptr; }

        // Function parameters
        while (ctx.peek().type != TokenType::PAREN_END) {
            if (auto arg = ctx.try_parse<FunctionArgument>()) {
                func_declaration->arguments.emplace_back(std::move(arg));
            } else {
                return nullptr;
            }

            if (!ctx.expect(TokenType::COMMA)) { break; }
            // closing parenthesis after a comma - invalid
            if (ctx.match(TokenType::PAREN_END)) {
                ctx.add_error("Misplaced comma");
                return nullptr;
            }
        }

        if (!ctx.expect(TokenType::PAREN_END)) { return nullptr; }
        if (ctx.expect(TokenType::ARROW)) {
            if (!ctx.parse_into<Type>(ctx, func_declaration->return_type)) {
                return nullptr;
            }
        } else {
            // implicit void return type
            func_declaration->return_type = Type::VOID;
        }

        if (!ctx.parse_into<BlockStatement>(ctx, func_declaration->body)) {
            return nullptr;
        }
        return func_declaration;
    }
};

template <>
struct ast::NodeParser<ast::Declaration> {
    static std::unique_ptr<Declaration> parse(Parser& ctx) {
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::FN: {
                auto func_declaration = ctx.try_parse<FunctionDeclaration>();
                return func_declaration;
            }
            case TokenType::INT: {
                auto variable_decl = ctx.try_parse<VariableDeclaration>();
                return variable_decl;
            }
            default: {
                return nullptr;
            }
        }
    }
};

template <>
struct ast::NodeParser<ast::Statement> {
    static std::unique_ptr<Statement> parse(Parser& ctx) {
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::IF:
                return ctx.try_parse<IfStatement>();

            case TokenType::WHILE:
                return ctx.try_parse<WhileStatement>();

            case TokenType::RETURN:
                return ctx.try_parse<ReturnStatement>();

            case TokenType::CURLY_BRACE_START:
                return ctx.try_parse<BlockStatement>();

            case TokenType::IDENTIFIER:
                if (auto assignment_statement = ctx.try_parse<AssignmentStatement>()) {
                    return assignment_statement;
                }
                [[fallthrough]];
            default:
                return ctx.try_parse<ExpressionStatement>();
        }
    }
};
