#include "lexer.hpp"

#include <cassert>
#include <iostream>
#include <map>

#include "utils/context.hpp"

std::vector<Token> lxr::Lexer::lex() {
    while (const int cur = peek()) {
        switch (cur) {
            case EOF: {
                emit_token(TokenType::TOKEN_EOF);
                return std::move(m_tokens);
            }
            case '\r': {
                this->consume();
                if (this->peek() == '\n') { this->consume(); }
                m_column = 0;
                m_line++;
                m_ctx.new_line(m_buffer.consumed);
                break;
            }
            case '\n': {
                this->consume();
                m_column = 0;
                m_line++;
                m_ctx.new_line(m_buffer.consumed);
                break;
            }
            // --- simple braces & punctuation ---
            case '{': {
                emit_token(TokenType::CURLY_BRACE_START);
                break;
            }
            case '}': {
                emit_token(TokenType::CURLY_BRACE_END);
                break;
            }
            case '(': {
                emit_token(TokenType::PAREN_START);
                break;
            }
            case ')': {
                emit_token(TokenType::PAREN_END);
                break;
            }
            case ';': {
                emit_token(TokenType::SEMICOLON);
                break;
            }
            case ':': {
                emit_token(TokenType::COLON);
                break;
            }
            case '.': {
                emit_token(TokenType::DOT);
                break;
            }
            case ',': {
                emit_token(TokenType::COMMA);
                break;
            }
            // --- operators ---
            case '+': {
                emit_token(TokenType::PLUS);
                break;
            }
            case '-': {
                if (peek(1) == '>') {
                    emit_token(TokenType::ARROW, 2);
                    break;
                }
                emit_token(TokenType::MINUS);
                break;
            }
            case '*': {
                emit_token(TokenType::ASTERISK);
                break;
            }
            case '/': {
                if (peek(1) == '/') {
                    this->consume(2);
                    const auto comment_len = get_comment_len();
                    this->consume(comment_len);
                    break;
                }
                emit_token(TokenType::DIVIDE);
                break;
            }
            case '>': {
                if (peek(1) == '=') {
                    emit_token(TokenType::GREATER_THAN, 2);
                    break;
                }
                emit_token(TokenType::GREATER_THAN);
                break;
            }
            case '<': {
                if (peek(1) == '=') {
                    emit_token(TokenType::LESS_EQUAL_THAN, 2);
                    break;
                }
                emit_token(TokenType::LESS_THAN);
                break;
            }
            case '=': {
                if (peek(1) == '=') {
                    emit_token(TokenType::EQUALS_EQUALS, 2);
                    break;
                }
                emit_token(TokenType::EQUALS);
                break;
            }
            // --- bitwise operators ---
            case '&': {
                if (peek(1) == '&') {
                    emit_token(TokenType::LOGICAL_AND, 2);
                    break;
                }
                emit_token(TokenType::BIT_AND);
                break;
            }
            case '|': {
                if (peek(1) == '|') {
                    emit_token(TokenType::LOGICAL_OR, 2);
                    break;
                }
                emit_token(TokenType::BIT_OR);
                break;
            }
            case '^': {
                emit_token(TokenType::BIT_XOR);
                break;
            }
            case '~': {
                emit_token(TokenType::BIT_NOT);
                break;
            }
            // --- logical operators ---
            case '!': {
                if (peek(1) == '=') {
                    emit_token(TokenType::NOT_EQUALS, 2);
                    break;
                }
                emit_token(TokenType::LOGICAL_NOT);
                break;
            }
            default: {
                if (is_digit(cur)) {
                    const auto digit_len = get_digit_len();
                    const auto digit_string = m_buffer.get_view(digit_len);
                    emit_token(
                        TokenType::INTEGER_LITERAL, digit_len, std::string(digit_string)
                    );
                    break;
                }
                if (is_identifier_start(cur)) {
                    const auto ident_len = get_identifier_len();
                    const auto ident_name = m_buffer.get_view(ident_len);
                    const auto type = get_keyword(ident_name);
                    if (type == TokenType::IDENTIFIER) {
                        emit_token(
                            TokenType::IDENTIFIER, ident_len, std::string(ident_name)
                        );
                        break;
                    }
                    emit_token(type, ident_len);
                    break;
                }
                if (is_whitespace_char(cur)) {
                    const auto whitespace_len = get_whitespace_len();
                    this->consume(whitespace_len);
                    break;
                }
                std::cerr << "Encountered unknown lexeme\n";
                this->consume(1);
                break;
            }
        }
    }
    return std::move(m_tokens);
}

std::string_view lxr::Lexer::PeekBuffer::get_view(const size_t n) {
    if (const size_t chars_left = count - current_pos; n > chars_left) {
        throw std::runtime_error("Attempted to get view past end of buffer");
    }
    return {&buffer[current_pos], n};
}

char lxr::Lexer::PeekBuffer::peek(const size_t offset) const {
    if (const size_t chars_left = count - current_pos; offset >= chars_left) {
        throw std::runtime_error("Attempted to look past the end of buffer");
    }
    return buffer[current_pos + offset];
}

void lxr::Lexer::PeekBuffer::consume(const size_t n) {
    const size_t chars_left = count - current_pos;
    const size_t consume = std::min(n, chars_left);
    current_pos += consume;
    consumed += consume;
    if (current_pos == count) {
        current_pos = 0;
        count = 0;
        empty = true;
    }
}

void lxr::Lexer::PeekBuffer::read_from(std::istream& input) {
    assert(empty);
    current_pos = 0;
    count = input.read(buffer.data(), BUFFER_SIZE).gcount();
    empty = (count == 0);
}

void lxr::Lexer::emit_token(
    const TokenType type, const std::optional<int> size, const TokenData& data
) {
    m_tokens.emplace_back(data, type, m_line, m_column);
    if (size.has_value()) {
        this->consume(*size);
    } else {
        this->consume();
    }
}

int lxr::Lexer::peek(const size_t offset) {
    if (m_buffer.empty) {
        m_buffer.read_from(m_input_stream);
        if (m_buffer.empty) { return EOF; }
    }
    return static_cast<unsigned char>(m_buffer.peek(offset));
}
void lxr::Lexer::consume(const size_t n) {
    this->m_column += static_cast<int>(n);
    m_buffer.consume(n);
}

size_t lxr::Lexer::get_digit_len() const {
    size_t digit_len = 0;
    while (is_digit(m_buffer.peek(digit_len))) { digit_len++; }
    return digit_len;
}

size_t lxr::Lexer::get_whitespace_len() const {
    size_t whitespace_len = 0;
    while (is_whitespace_char(m_buffer.peek(whitespace_len))) { whitespace_len++; }
    return whitespace_len;
}

size_t lxr::Lexer::get_identifier_len() const {
    size_t identifier_len = 0;
    if (!is_identifier_start(m_buffer.peek(identifier_len))) { return identifier_len; }
    while (is_identifier_char(m_buffer.peek(identifier_len))) { identifier_len++; }
    return identifier_len;
}

size_t lxr::Lexer::get_comment_len() const {
    size_t comment_len = 0;
    while (m_buffer.peek(comment_len) != '\r' && m_buffer.peek(comment_len) != '\n') {
        comment_len++;
    }
    return comment_len;
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

std::vector<Token> lxr::lex(std::istream& input, CompilationContext& ctx) {
    Lexer lexer(input, ctx);
    return lexer.lex();
}