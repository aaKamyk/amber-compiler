#include "parser.hpp"

#include <format>
#include <iostream>

#include "pass.hpp"

ast::Program<type::ast::Type> ast::Parser::parse() {
    Program<type::ast::Type> program{};

    while (peek().type != TokenType::TOKEN_EOF) {
        if (auto decl = try_parse<Declaration<type::ast::Type>>()) {
            program.children.emplace_back(std::move(decl));
            continue;
        }

        if (auto statement = try_parse<Statement<type::ast::Type>>()) {
            /// TODO Statements not allowed at global scope
            continue;
        }
        m_successfully_parsed = false;
        break;
    }

    return program;
}

void ast::Parser::consume(const size_t n) {
    if (const size_t tokens_left = m_tokens.size() - m_current_token; n > tokens_left) {
        throw std::runtime_error("Attempted to consume more tokens that available");
    }
    m_current_token += n;
}

bool ast::Parser::expect(const TokenType type) {
    if (peek().type == type) {
        consume();
        return true;
    }
    return false;
}

bool ast::Parser::match(const TokenType type, const size_t offset) const {
    return peek(offset).type == type;
}

void ast::Parser::restore(const size_t index) {
    m_current_token = index;
}

Token ast::Parser::peek(const size_t offset) const {
    if (m_current_token + offset >= m_tokens.size()) {
        throw std::runtime_error("Attempted to peek past available tokens");
    }
    return m_tokens[m_current_token + offset];
}

Token ast::Parser::current_token() const {
    return m_tokens[m_current_token];
}

void ast::Parser::add_error(std::string message, const bool add_context) {
    auto& errors = this->m_ctx.syntax_errors();
    const auto token = this->peek();
    if (!errors.empty() && errors.back().token_num == m_current_token) {
        errors.pop_back();
    }
    if (add_context) {
        if (token.type == TokenType::TOKEN_EOF) {
            message += " at the end of input";
        } else {
            message += " before \'" + token.to_string() + '\'';
        }
    }
    errors.emplace_back(token.line, token.column, message, this->m_current_token);
}

ast::Program<type::TypeID> ast::parse(
    std::vector<Token> tokens, CompilationContext& ctx
) {
    Parser parser(std::move(tokens), ctx);
    auto program = parser.parse();
    if (!parser.was_successful()) {
        for (const auto& error : parser.get_errors()) { error.print(&ctx); }
    }

    /// Collect type information
    CollectTypes type_collector(std::move(program));
    program = type_collector.run();

    /// Resolve types
    ResolveTypes type_resolver(std::move(program));
    auto resolved_program = type_resolver.run();

    /// Run constant fold pass
    ConstantFold constant_fold(std::move(resolved_program));
    auto folded_program = constant_fold.run();

    return folded_program;
}