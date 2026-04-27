#pragma once

#include <optional>
#include <vector>

#include "ast.hpp"
#include "lexer/token.hpp"
#include "symbolTable.hpp"

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
        : m_symbol_table(std::make_unique<SymbolTable>()), m_tokens(std::move(tokens)) {}

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

    std::unique_ptr<SymbolTable> m_symbol_table;
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
        destination = std::move(res);
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
        if (auto expr = ctx.try_parse<ASTExpression>()) {
            paren_expr.expression = std::move(*expr);
        } else {
            return std::nullopt;
        }
        if (!ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }
        return std::make_unique<ASTParenExpression>(std::move(paren_expr));
    }
};

inline std::optional<std::unique_ptr<ASTExpression>> ASTParser<ASTExpression>::parse(
    Parser& ctx, const int min_precedence
) {
    ASTExpression result{};
    if (auto primary_expr = ctx.try_parse<ASTPrimaryExpression>()) {
        result.expression = std::move(*primary_expr);
    } else {
        return std::nullopt;
    }
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
        if (auto unary_op = ctx.try_parse<ASTUnaryOperator>()) {
            unary_expr.op = std::move(*unary_op);
        } else {
            return std::nullopt;
        }
        if (auto primary_expr = ctx.try_parse<ASTPrimaryExpression>()) {
            unary_expr.expression = std::move(*primary_expr);
        } else {
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
                block_statement.statements.emplace_back(std::move(*decl));
            } else if (auto statement = ctx.try_parse<ASTStatement>()) {
                block_statement.statements.emplace_back(std::move(*statement));
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
        if (auto ident = ctx.try_parse<ASTIdentifier>()) {
            assignment_statement.identifier = std::move(*ident);
        } else {
            return std::nullopt;
        }
        if (auto expr = ctx.try_parse<ASTExpression>()) {
            assignment_statement.expression = std::move(*expr);
        } else {
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

        auto condition = ctx.try_parse<ASTExpression>();
        if (!condition) { return std::nullopt; }
        if_statement.condition = std::move(*condition);

        if (!ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }

        auto true_case = ctx.try_parse<ASTBlockStatement>();
        if (!true_case) { return std::nullopt; }
        if_statement.true_case = std::move(*true_case);

        if (ctx.expect(TokenType::ELSE)) {
            auto false_case = ctx.try_parse<ASTBlockStatement>();
            if (!false_case) { return std::nullopt; }
            if_statement.false_case = std::move(*false_case);
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
        // Type
        auto type = ctx.try_parse<ASTType>();
        if (!type) { return std::nullopt; }
        variable_declaration.type = std::move(*type);
        // Identifier
        auto ident = ctx.try_parse<ASTIdentifier>();
        if (!ident) { return std::nullopt; }
        variable_declaration.name = std::move(*ident);
        if (ctx.expect(TokenType::SEMICOLON)) {
            variable_declaration.value = nullptr;
            return std::make_unique<ASTVariableDeclaration>(std::move(variable_declaration));
        }
        if (ctx.expect(TokenType::EQUALS)) {
            auto expr = ctx.try_parse<ASTExpression>();
            if (!expr) { return std::nullopt; }
            variable_declaration.value = std::move(*expr);
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
        // Type
        auto type = ctx.try_parse<ASTType>();
        if (!type) { return std::nullopt; }
        arg.type = std::move(*type);
        // Identifier
        auto ident = ctx.try_parse<ASTIdentifier>();
        if (!ident) { return std::nullopt; }
        arg.name = std::move(*ident);
        return std::make_unique<ASTFunctionArgument>(std::move(arg));
    }
};

template <>
struct ASTParser<ASTFunctionDeclaration> {
    static std::optional<std::unique_ptr<ASTFunctionDeclaration>> parse(Parser& ctx) {
        ASTFunctionDeclaration func_declaration;
        if (!ctx.expect(TokenType::FN)) { return std::nullopt; }

        auto func_name = ctx.try_parse<ASTIdentifier>();
        if (!func_name) { return std::nullopt; }
        func_declaration.name = std::move(*func_name);

        if (ctx.expect(TokenType::PAREN_START)) { return std::nullopt; }

        // Function parameters
        while (ctx.peek().type != TokenType::PAREN_END) {
            if (auto arg = ctx.try_parse<ASTFunctionArgument>()) {
                func_declaration.arguments.push_back(std::move(*arg));
            } else {
                return std::nullopt;
            }

            if (!ctx.expect(TokenType::COMMA)) { break; }
            // closing parenthesis after a comma - invalid
            if (!ctx.match(TokenType::PAREN_END)) { return std::nullopt; }
        }

        if (ctx.expect(TokenType::PAREN_END)) { return std::nullopt; }
        if (ctx.expect(TokenType::ARROW)) {
            auto return_type = ctx.try_parse<ASTType>();
            if (!return_type) { return std::nullopt; }
            func_declaration.return_type = std::move(*return_type);
        } else {
            // implicit void return type
            func_declaration.return_type = std::make_unique<ASTType>(Type::VOID);
        }

        auto body = ctx.try_parse<ASTBlockStatement>();
        if (!body) { return std::nullopt; }
        func_declaration.body = std::move(*body);
        return std::make_unique<ASTFunctionDeclaration>(std::move(func_declaration));
    }
};

template <>
struct ASTParser<ASTDeclaration> {
    static std::optional<std::unique_ptr<ASTDeclaration>> parse(Parser& ctx) {
        if (ctx.match(TokenType::FN)) {
            auto function_decl = ctx.try_parse<ASTFunctionDeclaration>();
            if (!function_decl) { return std::nullopt; }
            return std::make_unique<ASTDeclaration>(std::move(*function_decl));
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
