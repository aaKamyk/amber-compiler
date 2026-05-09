#pragma once

#include <map>
#include <optional>
#include <vector>

#include "lexer/token.hpp"
#include "node.hpp"

namespace ast {
class Parser;

template <typename Node>
struct ASTParser {
    static std::unique_ptr<Node> parse(Parser& ctx) { return nullptr; }
};

template <>
struct ASTParser<Expression> {
    static std::unique_ptr<Expression> parse(Parser& ctx, int min_precedence = 0);
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {}

    Program parse();

private:
    template <typename T>
    friend struct ASTParser;

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
    [[nodiscard]] bool match(TokenType type) const;

    void restore(size_t index);
    [[nodiscard]] size_t save() const { return m_current_token; }
    [[nodiscard]] Token peek(size_t offset = 0) const;

    std::vector<Token> m_tokens;
    size_t m_current_token{};

    template <typename T>
    std::unique_ptr<T> try_parse() {
        const auto start = save();
        if (auto parse_result = ASTParser<T>::parse(*this)) { return parse_result; }
        restore(start);
        return nullptr;
    }

    template <typename T>
        requires std::same_as<T, BinaryOperator> || std::same_as<T, UnaryOperator> ||
                 std::same_as<T, Type>
    std::optional<T> try_parse() {
        const auto start = save();
        if (auto parse_result = ASTParser<T>::parse(*this)) { return parse_result; }
        restore(start);
        return std::nullopt;
    }

    std::unique_ptr<Expression> try_parse(const int precedence = 0) {
        const auto start = save();
        if (auto expr = ASTParser<Expression>::parse(*this, precedence)) { return expr; }
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

    template <typename T, typename... Types>
        requires(std::same_as<T, Types> || ...)
    bool parse_into(Parser& ctx, std::variant<std::unique_ptr<Types>...>& destination) {
        auto res = ctx.try_parse<T>();
        if (!res) { return false; }
        destination = std::move(*res);
        return true;
    }
};

Program parse(std::vector<Token> tokens);
}  // namespace ast

template <>
struct ast::ASTParser<Type> {
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
                return std::nullopt;
        }
    }
};

template <>
struct ast::ASTParser<AtomIdentifier> {
    static std::unique_ptr<AtomIdentifier> parse(Parser& ctx) {
        auto token = ctx.peek();
        if (!ctx.expect(TokenType::IDENTIFIER)) { return nullptr; }
        return std::make_unique<AtomIdentifier>(
            std::move(std::get<std::string>(token.data))
        );
    }
};

template <>
struct ast::ASTParser<AtomIntegerLiteral> {
    static std::unique_ptr<AtomIntegerLiteral> parse(Parser& ctx) {
        const auto token = ctx.peek();
        if (token.type != TokenType::INTEGER_LITERAL) { return nullptr; }
        ctx.consume(1);
        return std::make_unique<AtomIntegerLiteral>(
            std::stol(std::get<std::string>(token.data))
        );
    }
};

template <>
struct ast::ASTParser<ast::BinaryOperator> {
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
struct ast::ASTParser<ast::UnaryOperator> {
    static std::optional<UnaryOperator> parse(Parser& ctx) {
        /// TODO add the rest of the unary operators also make this expandable
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::MINUS:
                ctx.consume(1);
                return UnaryOperator::MINUS;
            default:
                return std::nullopt;
        }
    }
};

template <>
struct ast::ASTParser<ast::PrimaryExpression> {
    static std::unique_ptr<PrimaryExpression> parse(Parser& ctx) {
        if (auto atom_expr = ctx.try_parse<AtomExpression>()) { return atom_expr; }
        if (auto paren_expr = ctx.try_parse<ParenExpression>()) { return paren_expr; }
        if (auto unary_expr = ctx.try_parse<UnaryExpression>()) { return unary_expr; }
        return nullptr;
    }
};

template <>
struct ast::ASTParser<ast::AtomExpression> {
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
struct ast::ASTParser<ast::ParenExpression> {
    static std::unique_ptr<ParenExpression> parse(Parser& ctx) {
        auto paren_expr = std::make_unique<ParenExpression>();
        if (!ctx.expect(TokenType::PAREN_START)) { return nullptr; }
        if (!ctx.parse_into<Expression>(ctx, paren_expr->expression)) { return nullptr; }
        if (!ctx.expect(TokenType::PAREN_END)) { return nullptr; }
        return paren_expr;
    }
};

template <>
struct ast::ASTParser<ast::UnaryExpression> {
    static std::unique_ptr<UnaryExpression> parse(Parser& ctx) {
        auto unary_expr = std::make_unique<UnaryExpression>();
        if (!ctx.parse_into<UnaryOperator>(ctx, unary_expr->op)) { return nullptr; }
        if (!ctx.parse_into<PrimaryExpression>(ctx, unary_expr->expression)) {
            return nullptr;
        }
        return unary_expr;
    }
};

inline std::unique_ptr<ast::Expression> ast::ASTParser<ast::Expression>::parse(
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
struct ast::ASTParser<ast::BlockStatement> {
    static std::unique_ptr<BlockStatement> parse(Parser& ctx) {
        auto block_statement = std::make_unique<BlockStatement>();
        if (!ctx.expect(TokenType::CURLY_BRACE_START)) { return nullptr; }
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
struct ast::ASTParser<ast::AssignmentStatement> {
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
struct ast::ASTParser<ast::IfStatement> {
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
struct ast::ASTParser<ast::WhileStatement> {
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
struct ast::ASTParser<ast::ReturnStatement> {
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
struct ast::ASTParser<ast::VariableDeclaration> {
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
            if (!ctx.expect(TokenType::SEMICOLON)) { return nullptr; }
            return variable_declaration;
        }
        return nullptr;
    }
};

template <>
struct ast::ASTParser<ast::FunctionArgument> {
    static std::unique_ptr<FunctionArgument> parse(Parser& ctx) {
        auto arg = std::make_unique<FunctionArgument>();
        if (!ctx.parse_into<Type>(ctx, arg->type)) { return nullptr; }
        if (!ctx.parse_into<AtomIdentifier>(ctx, arg->name)) { return nullptr; }
        return arg;
    }
};

template <>
struct ast::ASTParser<ast::FunctionDeclaration> {
    static std::unique_ptr<FunctionDeclaration> parse(Parser& ctx) {
        auto func_declaration = std::make_unique<ast::FunctionDeclaration>();
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
            if (!ctx.match(TokenType::PAREN_END)) { return nullptr; }
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
struct ast::ASTParser<ast::Declaration> {
    static std::unique_ptr<Declaration> parse(Parser& ctx) {
        if (ctx.match(TokenType::FN)) {
            if (auto func_declaration = ctx.try_parse<FunctionDeclaration>()) {
                return func_declaration;
            }
        }
        if (auto variable_decl = ctx.try_parse<VariableDeclaration>()) {
            return variable_decl;
        }
        return nullptr;
    }
};

template <>
struct ast::ASTParser<ast::Statement> {
    static std::unique_ptr<Statement> parse(Parser& ctx) {
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::IF: {
                auto if_statement = ctx.try_parse<IfStatement>();
                return if_statement;
            }
            case TokenType::WHILE: {
                auto while_statement = ctx.try_parse<WhileStatement>();
                return while_statement;
            }
            case TokenType::RETURN: {
                auto return_statement = ctx.try_parse<ReturnStatement>();
                return return_statement;
            }
            case TokenType::CURLY_BRACE_START: {
                auto block_statement = ctx.try_parse<BlockStatement>();
                return block_statement;
            }
            case TokenType::IDENTIFIER: {
                auto assignment_statement = ctx.try_parse<AssignmentStatement>();
                return assignment_statement;
            }
            default: {
                return nullptr;
            }
        }
    }
};
