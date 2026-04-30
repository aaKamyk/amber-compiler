#pragma once

#include <optional>
#include <vector>

#include "ast.hpp"
#include "lexer/token.hpp"

class Parser;

template <typename ASTNode>
struct ASTParser {
    static std::optional<std::unique_ptr<ASTNode>> parse(Parser& ctx) { return std::nullopt; }
};

template <>
struct ASTParser<ASTExpression> {
    static std::optional<std::unique_ptr<ASTExpression>> parse(Parser& ctx, int min_precedence = 0);
};

class Parser {
public:
    explicit Parser(std::vector<Token>&& tokens)
        : m_tokens(std::move(tokens)) {}

    ASTProgram parse();

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
    bool match(TokenType type) const;

    void restore(size_t index);
    [[nodiscard]] size_t save() const { return m_current_token; }
    [[nodiscard]] Token peek(size_t offset = 0) const;

    std::vector<Token> m_tokens;
    size_t m_current_token{};

    template <typename T>
    std::optional<std::unique_ptr<T>> try_parse() {
        const auto start = save();
        if (auto parse_result = ASTParser<T>::parse(*this)) { return std::move(*parse_result); }
        restore(start);
        return std::nullopt;
    }

    std::optional<std::unique_ptr<ASTExpression>> try_parse(const int precedence = 0) {
        const auto start = save();
        if (auto expr = ASTParser<ASTExpression>::parse(*this, precedence)) { return expr; }
        restore(start);
        return std::nullopt;
    }

    template <typename T>
    bool parse_into(Parser& ctx, std::unique_ptr<T>& destination) {
        auto res = ctx.try_parse<T>();
        if (!res) { return false; }
        destination = std::move(*res);
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

template <>
struct ASTParser<ASTType> {
    static std::optional<std::unique_ptr<ASTType>> parse(Parser& ctx) {
        /// TODO make this expandable
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::INT:
                ctx.consume(1);
                return std::make_unique<ASTType>(Type::INT);
            case TokenType::VOID:
                ctx.consume(1);
                return std::make_unique<ASTType>(Type::VOID);
            default:
                return std::nullopt;
        }
    }
};

template <>
struct ASTParser<ASTIdentifier> {
    static std::optional<std::unique_ptr<ASTIdentifier>> parse(Parser& ctx) {
        auto token = ctx.peek();
        if (token.type != TokenType::IDENTIFIER) { return std::nullopt; }
        ctx.consume(1);
        return std::make_unique<ASTIdentifier>(std::move(std::get<std::string>(token.data)));
    }
};

template <>
struct ASTParser<ASTIntegerLiteral> {
    static std::optional<std::unique_ptr<ASTIntegerLiteral>> parse(Parser& ctx) {
        const auto token = ctx.peek();
        if (token.type != TokenType::INTEGER_LITERAL) { return std::nullopt; }
        ctx.consume(1);
        return std::make_unique<ASTIntegerLiteral>(std::stol(std::get<std::string>(token.data)));
    }
};

template <>
struct ASTParser<ASTBinaryOperator> {
    static std::optional<std::unique_ptr<ASTBinaryOperator>> parse(Parser& ctx) {
        /// TODO add the rest of the binary operators also make this expandable
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::PLUS:
                ctx.consume(1);
                return std::make_unique<ASTBinaryOperator>(BinaryOperator::PLUS);
            case TokenType::MINUS:
                ctx.consume(1);
                return std::make_unique<ASTBinaryOperator>(BinaryOperator::MINUS);
            case TokenType::ASTERISK:
                ctx.consume(1);
                return std::make_unique<ASTBinaryOperator>(BinaryOperator::TIMES);
            case TokenType::DIVIDE:
                ctx.consume(1);
                return std::make_unique<ASTBinaryOperator>(BinaryOperator::DIVIDE);
            default:
                return std::nullopt;
        }
    }
};

template <>
struct ASTParser<ASTUnaryOperator> {
    static std::optional<std::unique_ptr<ASTUnaryOperator>> parse(Parser& ctx) {
        /// TODO add the rest of the unary operators also make this expandable
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::MINUS:
                ctx.consume(1);
                return std::make_unique<ASTUnaryOperator>(UnaryOperator::MINUS);
            default:
                return std::nullopt;
        }
    }
};

template <>
struct ASTParser<ASTPrimaryExpression> {
    static std::optional<std::unique_ptr<ASTPrimaryExpression>> parse(Parser& ctx) {
        if (auto ident = ctx.try_parse<ASTIdentifier>()) {
            return std::make_unique<ASTPrimaryExpression>(std::move(*ident));
        }
        if (auto int_literal = ctx.try_parse<ASTIntegerLiteral>()) {
            return std::make_unique<ASTPrimaryExpression>(std::move(*int_literal));
        }
        if (auto paren_expr = ctx.try_parse<ASTParenExpression>()) {
            return std::make_unique<ASTPrimaryExpression>(std::move(*paren_expr));
        }
        if (auto unary_expr = ctx.try_parse<ASTUnaryExpression>()) {
            return std::make_unique<ASTPrimaryExpression>(std::move(*unary_expr));
        }
        return std::nullopt;
    }
};

template <>
struct ASTParser<ASTParenExpression> {
    static std::optional<std::unique_ptr<ASTParenExpression>> parse(Parser& ctx) {
        ASTParenExpression paren_expr{};
        if (!ctx.expect(TokenType::PAREN_START)) { return std::nullopt; }
        if (!ctx.parse_into<ASTExpression>(ctx, paren_expr.expression)) { return std::nullopt; }
        if (!ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }
        return std::make_unique<ASTParenExpression>(std::move(paren_expr));
    }
};

inline std::optional<std::unique_ptr<ASTExpression>> ASTParser<ASTExpression>::parse(
    Parser& ctx, const int min_precedence
) {
    ASTExpression result{};
    if (!ctx.parse_into<ASTPrimaryExpression>(ctx, result.expression)) { return std::nullopt; }
    while (true) {
        const auto start = ctx.save();
        auto bin_op = ctx.try_parse<ASTBinaryOperator>();
        if (!bin_op || (*bin_op)->get_precedence() <= min_precedence) {
            ctx.restore(start);
            break;
        }
        auto right = ctx.try_parse((*bin_op)->get_precedence());
        if (!right) { return std::nullopt; }
        // result becomes the left side of the binary expression
        auto bin_expr = std::make_unique<ASTBinaryExpression>(
            std::move(*bin_op), std::make_unique<ASTExpression>(std::move(result)),
            std::move(*right)
        );
        result.expression = std::move(bin_expr);
    }
    return std::make_unique<ASTExpression>(std::move(result));
}

template <>
struct ASTParser<ASTUnaryExpression> {
    static std::optional<std::unique_ptr<ASTUnaryExpression>> parse(Parser& ctx) {
        ASTUnaryExpression unary_expr{};
        if (!ctx.parse_into<ASTUnaryOperator>(ctx, unary_expr.op)) { return std::nullopt; }
        if (!ctx.parse_into<ASTPrimaryExpression>(ctx, unary_expr.expression)) {
            return std::nullopt;
        }
        return std::make_unique<ASTUnaryExpression>(std::move(unary_expr));
    }
};

template <>
struct ASTParser<ASTBlockStatement> {
    static std::optional<std::unique_ptr<ASTBlockStatement>> parse(Parser& ctx) {
        ASTBlockStatement block_statement;
        if (!ctx.expect(TokenType::CURLY_BRACE_START)) { return std::nullopt; }
        while (true) {
            if (auto decl = ctx.try_parse<ASTDeclaration>()) {
                block_statement.entries.emplace_back(std::move(*decl));
            } else if (auto statement = ctx.try_parse<ASTStatement>()) {
                block_statement.entries.emplace_back(std::move(*statement));
            } else {
                break;
            }
        }
        if (!ctx.expect(TokenType::CURLY_BRACE_END)) { return std::nullopt; }
        return std::make_unique<ASTBlockStatement>(std::move(block_statement));
    }
};

template <>
struct ASTParser<ASTAssignmentStatement> {
    static std::optional<std::unique_ptr<ASTAssignmentStatement>> parse(Parser& ctx) {
        ASTAssignmentStatement assignment_statement;
        if (!ctx.parse_into<ASTIdentifier>(ctx, assignment_statement.identifier)) {
            return std::nullopt;
        }
        if (!ctx.parse_into<ASTExpression>(ctx, assignment_statement.expression)) {
            return std::nullopt;
        }
        return std::make_unique<ASTAssignmentStatement>(std::move(assignment_statement));
    }
};

template <>
struct ASTParser<ASTIfStatement> {
    static std::optional<std::unique_ptr<ASTIfStatement>> parse(Parser& ctx) {
        ASTIfStatement if_statement{};
        if (!ctx.expect(TokenType::IF, TokenType::PAREN_START)) { return std::nullopt; }

        if (!ctx.parse_into<ASTExpression>(ctx, if_statement.condition)) { return std::nullopt; }
        if (!ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }
        if (!ctx.parse_into<ASTBlockStatement>(ctx, if_statement.true_case)) {
            return std::nullopt;
        }
        if (ctx.expect(TokenType::ELSE)) {
            if (!ctx.parse_into<ASTBlockStatement>(ctx, if_statement.false_case)) {
                return std::nullopt;
            }
            return std::make_unique<ASTIfStatement>(std::move(if_statement));
        }
        if_statement.false_case = nullptr;
        return std::make_unique<ASTIfStatement>(std::move(if_statement));
    }
};

template <>
struct ASTParser<ASTVariableDeclaration> {
    static std::optional<std::unique_ptr<ASTVariableDeclaration>> parse(Parser& ctx) {
        ASTVariableDeclaration variable_declaration;
        if (!ctx.parse_into<ASTType>(ctx, variable_declaration.type)) { return std::nullopt; }
        if (!ctx.parse_into<ASTIdentifier>(ctx, variable_declaration.name)) { return std::nullopt; }
        if (ctx.expect(TokenType::SEMICOLON)) {
            variable_declaration.value = nullptr;
            return std::make_unique<ASTVariableDeclaration>(std::move(variable_declaration));
        }
        if (ctx.expect(TokenType::EQUALS)) {
            if (!ctx.parse_into<ASTExpression>(ctx, variable_declaration.value)) {
                return std::nullopt;
            }
            if (!ctx.expect(TokenType::SEMICOLON)) { return std::nullopt; }
            return std::make_unique<ASTVariableDeclaration>(std::move(variable_declaration));
        }
        return std::nullopt;
    }
};

template <>
struct ASTParser<ASTFunctionArgument> {
    static std::optional<std::unique_ptr<ASTFunctionArgument>> parse(Parser& ctx) {
        ASTFunctionArgument arg{};
        if (!ctx.parse_into<ASTType>(ctx, arg.type)) { return std::nullopt; }
        if (!ctx.parse_into<ASTIdentifier>(ctx, arg.name)) { return std::nullopt; }
        return std::make_unique<ASTFunctionArgument>(std::move(arg));
    }
};

template <>
struct ASTParser<ASTFunctionDeclaration> {
    static std::optional<std::unique_ptr<ASTFunctionDeclaration>> parse(Parser& ctx) {
        ASTFunctionDeclaration func_declaration;
        if (!ctx.expect(TokenType::FN)) { return std::nullopt; }

        if (!ctx.parse_into<ASTIdentifier>(ctx, func_declaration.name)) { return std::nullopt; }
        if (!ctx.expect(TokenType::PAREN_START)) { return std::nullopt; }

        // Function parameters
        while (ctx.peek().type != TokenType::PAREN_END) {
            if (auto arg = ctx.try_parse<ASTFunctionArgument>()) {
                func_declaration.arguments.emplace_back(std::move(*arg));
            } else {
                return std::nullopt;
            }

            if (!ctx.expect(TokenType::COMMA)) { break; }
            // closing parenthesis after a comma - invalid
            if (!ctx.match(TokenType::PAREN_END)) { return std::nullopt; }
        }

        if (!ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }
        if (ctx.expect(TokenType::ARROW)) {
            if (!ctx.parse_into<ASTType>(ctx, func_declaration.return_type)) {
                return std::nullopt;
            }
        } else {
            // implicit void return type
            func_declaration.return_type = std::make_unique<ASTType>(Type::VOID);
        }

        if (!ctx.parse_into<ASTBlockStatement>(ctx, func_declaration.body)) { return std::nullopt; }
        return std::make_unique<ASTFunctionDeclaration>(std::move(func_declaration));
    }
};

template <>
struct ASTParser<ASTDeclaration> {
    static std::optional<std::unique_ptr<ASTDeclaration>> parse(Parser& ctx) {
        if (ctx.match(TokenType::FN)) {
            if (auto func_declaration = ctx.try_parse<ASTFunctionDeclaration>()) {
                return std::make_unique<ASTDeclaration>(std::move(*func_declaration));
            }
        }
        if (auto variable_decl = ctx.try_parse<ASTVariableDeclaration>()) {
            return std::make_unique<ASTDeclaration>(std::move(*variable_decl));
        }
        return std::nullopt;
    }
};

template <>
struct ASTParser<ASTStatement> {
    static std::optional<std::unique_ptr<ASTStatement>> parse(Parser& ctx) {
        switch (const auto token = ctx.peek(); token.type) {
            case TokenType::IF: {
                auto if_statement = ctx.try_parse<ASTIfStatement>();
                if (!if_statement) { return std::nullopt; }
                return std::make_unique<ASTStatement>(std::move(*if_statement));
            }
            case TokenType::WHILE: {
                auto while_statement = ctx.try_parse<ASTWhileStatement>();
                if (!while_statement) { return std::nullopt; }
                return std::make_unique<ASTStatement>(std::move(*while_statement));
            }
            case TokenType::RETURN: {
                auto return_statement = ctx.try_parse<ASTReturnStatement>();
                if (!return_statement) { return std::nullopt; }
                return std::make_unique<ASTStatement>(std::move(*return_statement));
            }
            case TokenType::CURLY_BRACE_START: {
                auto block_statement = ctx.try_parse<ASTBlockStatement>();
                if (!block_statement) { return std::nullopt; }
                return std::make_unique<ASTStatement>(std::move(*block_statement));
            }
            case TokenType::IDENTIFIER: {
                auto assignment_statement = ctx.try_parse<ASTAssignmentStatement>();
                if (!assignment_statement) { return std::nullopt; }
                return std::make_unique<ASTStatement>(std::move(*assignment_statement));
            }
            default: {
                return std::nullopt;
            }
        }
    }
};
