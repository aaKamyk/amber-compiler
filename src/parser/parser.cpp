#include "parser.hpp"

#include <format>
#include <iostream>

ast::Program ast::Parser::parse() {
    Program program{};
    while (peek().type != TokenType::TOKEN_EOF) {
        if (auto decl = try_parse<Declaration>()) {
            program.children.emplace_back(std::move(decl));
            continue;
        }
        if (auto statement = try_parse<Statement>()) {
            program.children.emplace_back(std::move(statement));
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

void ast::Parser::add_error(const std::string& message) {
    const auto token = this->peek();
    m_ctx.parser_errors().emplace_back(
        token.line, token.column, message, this->m_current_token
    );
}

ast::Program ast::parse(std::vector<Token> tokens, CompilationContext& ctx) {
    Parser parser(std::move(tokens), ctx);
    auto program = parser.parse();
    if (parser.was_successful()) { return program; }
    for (const auto& error : parser.get_errors()) {
        std::cout << ctx.get_path() << ':';
        error.print();
        std::cout << ctx.get_line(error.line) << '\n';
        std::string padding(error.column, ' ');
        std::cout << padding << "^\n";
    }
    return {};
}