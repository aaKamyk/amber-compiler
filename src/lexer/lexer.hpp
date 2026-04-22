#pragma once

#include <array>
#include <istream>
#include <optional>
#include <variant>
#include <vector>

#include "token.hpp"

class Lexer {
public:
    explicit Lexer(std::istream& input)
        : m_input_stream(input) {}

    std::vector<Token> lex();
    static void pretty_print_tokens(const std::vector<Token>& tokens);

private:
    struct PeekBuffer {
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
