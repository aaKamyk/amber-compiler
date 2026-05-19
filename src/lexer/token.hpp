#pragma once

#include <cstdint>
#include <string>
#include <variant>

enum class TokenType : uint8_t {
    // --- special ---
    TOKEN_EOF,
    NONE,
    // --- Keywords ---
    IF,
    ELSE,
    WHILE,
    FN,
    RETURN,
    // --- Types ---
    I32,
    VOID,
    // --- identifiers & literals ---
    IDENTIFIER,
    INTEGER_LITERAL,
    // --- braces & punctuation ---
    CURLY_BRACE_START,
    CURLY_BRACE_END,
    PAREN_START,
    PAREN_END,
    SEMICOLON,
    COLON,
    DOT,
    COMMA,
    ARROW,  // ->
    // --- operators ---
    PLUS,
    MINUS,
    ASTERISK,
    DIVIDE,
    EQUALS,
    EQUALS_EQUALS,
    NOT_EQUALS,
    GREATER_THAN,
    LESS_THAN,
    GREATER_EQUAL_THAN,
    LESS_EQUAL_THAN,
    // bitwise operators ---
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    // --- logical operators ---
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
};

using TokenData = std::variant<std::monostate, std::string>;

struct Token {
    TokenData data{std::monostate{}};
    TokenType type{};
    int line{};
    int column{};

    [[nodiscard]] std::string to_string() const;
};
