#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <optional>
#include <variant>
#include <vector>

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
    INT,
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
    MULTIPLY,
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

struct Token
{
    TokenData data{std::monostate{}};
    TokenType type{};
    int line{};
    int column{};
};

class Lexer {
public:
    explicit Lexer(std::istream& input)
        : m_input_stream(input) {}

    std::vector<Token> lex();
    static void pretty_print_tokens(const std::vector<Token>& tokens);

private:
    struct PeekBuffer
    {
        static constexpr size_t BUFFER_SIZE = 1 << 10;  // 1KB buffer
        std::array<char, BUFFER_SIZE> buffer{};
        size_t count{};
        size_t current_pos{};
        bool empty{true};

        [[nodiscard]]
        std::string_view get_view(size_t n);
        [[nodiscard]]
        char peek(size_t offset) const;
        void consume(size_t n);
        void read_from(std::istream& input);
    };

    void emit_token(TokenType type, std::optional<int> size = std::nullopt, const TokenData& data = std::monostate{});
    int peek(size_t offset = 0);

    [[nodiscard]] size_t get_digit_len() const;
    [[nodiscard]] size_t get_whitespace_len() const;
    [[nodiscard]] size_t get_identifier_len() const;

    static TokenType get_keyword(std::string_view name);
    static bool is_identifier_start(const int chr) { return std::isalpha(chr) != 0; }
    static bool is_identifier_char(const int chr) { return std::isalnum(chr) != 0; }
    static bool is_whitespace_char(const int chr) { return chr == ' ' || chr == '\t'; }
    static bool is_digit(const int chr) { return std::isdigit(chr) != 0; }

    PeekBuffer m_buffer{};
    std::vector<Token> m_tokens;
    std::istream& m_input_stream;
    int m_line{1};
    int m_column{0};
};
