#include "parser.hpp"

#include <format>

ast::Program Parser::parse() {
    ast::Program program{};
    while (peek().type != TokenType::TOKEN_EOF) {
        if (auto decl = try_parse<ast::Declaration>()) {
            program.children.emplace_back(std::move(decl));
            continue;
        }
        if (auto statement = try_parse<ast::Statement>()) {
            program.children.emplace_back(std::move(statement));
            continue;
        }
        throw std::runtime_error("Expected statement or declaration");
    }
    return program;
}

void Parser::consume(const size_t n) {
    if (const size_t tokens_left = m_tokens.size() - m_current_token; n > tokens_left) {
        throw std::runtime_error("Attempted to consume more tokens that available");
    }
    m_current_token += n;
}

bool Parser::expect(const TokenType type) {
    if (peek().type == type) {
        consume();
        return true;
    }
    return false;
}

bool Parser::match(const TokenType type) const {
    return peek().type == type;
}

void Parser::restore(const size_t index) {
    m_current_token = index;
}

Token Parser::peek(const size_t offset) const {
    if (m_current_token + offset >= m_tokens.size()) {
        throw std::runtime_error("Attempted to peek past available tokens");
    }
    return m_tokens[m_current_token + offset];
}