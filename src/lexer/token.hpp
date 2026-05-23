#pragma once

#include <cstdint>
#include <string>
#include <variant>

// clang-format off
enum class TokenType : uint8_t {
    // --- special ---
    TOKEN_EOF,
    NONE,
    // --- Keywords ---
    IF,         // statements
    ELSE,
    WHILE,
    FN,         // declarations
    LET,
    RETURN,
    TRUE,       // built-in values
    FALSE,
    // --- Types ---
    I64,
    I32,
    I16,
    I8,
    BOOL,
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
    // --- cast operator ---
    AS
};
// clang-format on

using TokenData = std::variant<std::monostate, std::string>;

struct Token {
    TokenData data{std::monostate{}};
    TokenType type{};
    int line{};
    int column{};

    [[nodiscard]] std::string to_string() const;
};
