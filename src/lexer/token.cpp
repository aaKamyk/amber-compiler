#include "token.hpp"

#include <map>
#include <stdexcept>

std::string Token::to_string() const {
    std::map<TokenType, const char*> token_strings = {
        // --- special ---
        {TokenType::TOKEN_EOF, "EOF"},
        {TokenType::NONE, "NONE"},
        // --- Keywords ---
        {TokenType::IF, "if"},
        {TokenType::ELSE, "else"},
        {TokenType::WHILE, "while"},
        {TokenType::FN, "fn"},
        {TokenType::RETURN, "return"},
        // --- Types ---
        {TokenType::INT, "int"},
        {TokenType::VOID, "void"},
        // --- braces & punctuation ---
        {TokenType::CURLY_BRACE_START, "{"},
        {TokenType::CURLY_BRACE_END, "}"},
        {TokenType::PAREN_START, "("},
        {TokenType::PAREN_END, ")"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::COLON, ":"},
        {TokenType::DOT, "."},
        {TokenType::COMMA, ","},
        {TokenType::ARROW, "->"},  // ->
        // --- operators ---
        {TokenType::PLUS, "+"},
        {TokenType::MINUS, "-"},
        {TokenType::ASTERISK, "*"},
        {TokenType::DIVIDE, "/"},
        {TokenType::EQUALS, "="},
        {TokenType::EQUALS_EQUALS, "=="},
        {TokenType::NOT_EQUALS, "!="},
        {TokenType::GREATER_THAN, ">"},
        {TokenType::LESS_THAN, "<"},
        {TokenType::GREATER_EQUAL_THAN, ">="},
        {TokenType::LESS_EQUAL_THAN, "<="},
        // bitwise operators ---
        {TokenType::BIT_AND, "&"},
        {TokenType::BIT_OR, "|"},
        {TokenType::BIT_XOR, "^"},
        {TokenType::BIT_NOT, "~"},
        // --- logical operators ---
        {TokenType::LOGICAL_AND, "&&"},
        {TokenType::LOGICAL_OR, "||"},
        {TokenType::LOGICAL_NOT, "!"},
    };
    switch (this->type) {
        case TokenType::IDENTIFIER:
        case TokenType::INTEGER_LITERAL: {
            return std::get<std::string>(this->data);
        }
        default: {
            if (const auto it = token_strings.find(this->type);
                it != token_strings.end()) {
                return it->second;
            }
            throw std::runtime_error("Unknown token type");
        }
    }
}