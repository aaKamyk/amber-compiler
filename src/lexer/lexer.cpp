#include "lexer.hpp"

#include <cassert>
#include <iostream>
#include <map>

#include "utils/context.hpp"

std::vector<Token> lxr::Lexer::lex() {
    while (true) {
        auto token = this->next_token();
        const bool is_end = token.type == TokenType::TOKEN_EOF;
        if (!token.is_ignorable()) { this->m_tokens.emplace_back(std::move(token)); }
        if (is_end) { return std::move(m_tokens); }
    }
}

Token lxr::Lexer::next_token() {
    const auto first_char = this->peek();
    if (first_char == EOF) { return this->consume_token(TokenType::TOKEN_EOF); }

    switch (first_char) {
        // --- whitespace & eof ---
        case EOF:
            return this->consume_token(TokenType::TOKEN_EOF);
        case '\r':
        case '\n':
            return this->consume_token(TokenType::WHITESPACE);
        // --- simple braces & punctuation ---
        case '{':
            return this->consume_token(TokenType::CURLY_BRACE_START);
        case '}':
            return this->consume_token(TokenType::CURLY_BRACE_END);
        case '(':
            return this->consume_token(TokenType::PAREN_START);
        case ')':
            return this->consume_token(TokenType::PAREN_END);
        case ';':
            return this->consume_token(TokenType::SEMICOLON);
        case ':':
            return this->consume_token(TokenType::COLON);
        case '.':
            return this->consume_token(TokenType::DOT);
        case ',':
            return this->consume_token(TokenType::COMMA);
        // --- operators ---
        case '+':
            return this->consume_token(TokenType::PLUS);
        case '-': {
            if (peek(1) == '>') {
                return this->consume_token(TokenType::ARROW, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::MINUS);
        }
        case '*':
            return this->consume_token(TokenType::ASTERISK);
        case '/': {
            switch (this->peek(1)) {
                case '/':
                    return this->comment_token();
                case '*':
                    return this->block_comment_token();
                default:
                    return this->consume_token(TokenType::DIVIDE);
            }
        }
        // --- logical operators ---
        case '>': {
            if (peek(1) == '=') {
                return this->consume_token(TokenType::GREATER_THAN, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::GREATER_THAN);
        }
        case '<': {
            if (peek(1) == '=') {
                return this->consume_token(
                    TokenType::LESS_EQUAL_THAN, std::monostate{}, 2
                );
            }
            return this->consume_token(TokenType::LESS_THAN);
        }
        case '=': {
            if (peek(1) == '=') {
                return this->consume_token(TokenType::EQUALS_EQUALS, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::EQUALS);
        }
        case '!': {
            if (peek(1) == '=') {
                return this->consume_token(TokenType::NOT_EQUALS, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::LOGICAL_NOT);
        }
        // --- bitwise operators ---
        case '&': {
            if (peek(1) == '&') {
                return this->consume_token(TokenType::LOGICAL_AND, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::BIT_AND);
        }
        case '|': {
            if (peek(1) == '|') {
                return this->consume_token(TokenType::LOGICAL_OR, std::monostate{}, 2);
            }
            return this->consume_token(TokenType::BIT_OR);
        }
        case '^':
            return this->consume_token(TokenType::BIT_XOR);
        case '~':
            return this->consume_token(TokenType::BIT_NOT);
        default: {
            if (is_digit(first_char)) {
                const auto digit_len = get_digit_len();
                const auto digit_span = this->get_view(digit_len).expect(
                    "Expected digit_len to be less than size of file"
                );
                auto digit_string = this->m_source_file.get_string(digit_span).unwrap();
                return this->consume_token(
                    TokenType::INTEGER_LITERAL, std::move(digit_string), digit_len
                );
            }
            if (is_identifier_start(first_char)) {
                const auto ident_len = this->get_identifier_len();
                const auto ident_span = this->get_view(ident_len).expect(
                    "Expected ident_len to be less than size of file"
                );
                const auto type = Lexer::get_keyword(
                    this->m_source_file.get_string_view(ident_span).unwrap()
                );
                if (type == TokenType::IDENTIFIER) {
                    return this->consume_token(
                        TokenType::IDENTIFIER,
                        this->m_source_file.get_string(ident_span).unwrap(), ident_len
                    );
                }
                return this->consume_token(type, std::monostate{}, ident_len);
            }
            if (is_whitespace_char(first_char)) {
                const auto whitespace_len = this->get_whitespace_len();
                return this->consume_token(
                    TokenType::WHITESPACE, std::monostate{}, whitespace_len
                );
            }

            std::cerr << "Encountered unknown lexeme\n";
            return this->consume_token(TokenType::NONE);
        }
    }
}

Token lxr::Lexer::consume_token(
    const TokenType type, TokenData data, const std::size_t len
) {
    /// TODO
    this->consume(len);
    return Token{.data = std::move(data), .type = type, .line = 0, .column = 0};
}

Token lxr::Lexer::comment_token() {
    assert(this->peek(0) == '/' && this->peek(1) == '/');

    this->consume(2);
    const auto comment_len = this->get_comment_len();
    return this->consume_token(TokenType::COMMENT, std::monostate{}, comment_len);
}

Token lxr::Lexer::block_comment_token() {
    assert(this->peek(0) == '/' && this->peek(1) == '*');

    this->consume(2);
    const auto block_comment_len = this->get_block_comment_len();
    return this->consume_token(
        TokenType::BLOCK_COMMENT, std::monostate{}, block_comment_len
    );
}

int lxr::Lexer::peek(const size_t offset) const {
    const std::size_t current = this->m_remaining.start;
    const std::size_t target = current + offset;

    if (target >= this->m_source_file.size()) { return EOF; }
    return static_cast<int>(this->m_source_file.bytes()[target]);
}

int lxr::Lexer::previous() const {
    const std::size_t current = this->m_remaining.start;
    if (current < 1) { return EOF; }

    return static_cast<int>(this->m_source_file.bytes()[current - 1]);
}

void lxr::Lexer::consume(const size_t n) {
    this->m_remaining.advance(n);
}

size_t lxr::Lexer::get_digit_len() const {
    size_t digit_len = 0;
    while (is_digit(this->peek(digit_len))) { digit_len++; }
    return digit_len;
}

size_t lxr::Lexer::get_whitespace_len() const {
    size_t whitespace_len = 0;
    while (is_whitespace_char(this->peek(whitespace_len))) { whitespace_len++; }
    return whitespace_len;
}

size_t lxr::Lexer::get_identifier_len() const {
    size_t identifier_len = 0;
    if (!is_identifier_start(this->peek(identifier_len))) { return identifier_len; }
    while (is_identifier_char(this->peek(identifier_len))) { identifier_len++; }
    return identifier_len;
}

size_t lxr::Lexer::get_comment_len() const {
    std::size_t comment_len = 0;
    while (this->peek(comment_len) != '\r' && this->peek(comment_len) != '\n') {
        comment_len++;
    }
    return comment_len;
}

std::size_t lxr::Lexer::get_block_comment_len() const {
    std::size_t block_comment_len = 0;
    while (this->peek(block_comment_len) != '*' && this->peek(block_comment_len) != '/') {
        block_comment_len++;
    }
    return block_comment_len;
}

Option<SourceSpan> lxr::Lexer::get_view(const std::size_t len) const {
    using namespace lib;

    const std::size_t current = this->m_remaining.start;
    if (const std::size_t end = current + len; end >= this->m_source_file.size()) {
        return option::none<SourceSpan>();
    }
    return option::some(SourceSpan{.start = current, .size = len});
}

TokenType lxr::Lexer::get_keyword(const std::string_view name) {
    static const std::map<std::string_view, TokenType> keywords = {
        /// statement keywords
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"return", TokenType::RETURN},
        /// declarations
        {"fn", TokenType::FN},
        {"let", TokenType::LET},
        {"struct", TokenType::STRUCT},
        /// built-in values
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
        /// built-in types
        {"i64", TokenType::I64},
        {"i32", TokenType::I32},
        {"i16", TokenType::I16},
        {"i8", TokenType::I8},
        {"bool", TokenType::BOOL},
        {"void", TokenType::VOID},
        /// cast operator
        {"as", TokenType::AS},
    };
    if (!keywords.contains(name)) { return TokenType::IDENTIFIER; }
    return keywords.at(name);
}

std::vector<Token> lxr::lex(const SourceFile& input, CompilationContext& ctx) {
    Lexer lexer(input, ctx);
    return lexer.lex();
}