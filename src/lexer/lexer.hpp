#pragma once

#include <array>
#include <optional>
#include <variant>
#include <vector>

#include "token.hpp"
#include "utils/context.hpp"

namespace lxr {
class Lexer {
public:
    explicit Lexer(const SourceFile& input, CompilationContext& context)
        : m_source_file(input), m_ctx(context) {
        this->m_remaining = {.start = 0, .size = m_source_file.size()};
    }

    std::vector<Token> lex();

private:
    void emit_token(
        TokenType type, std::optional<int> size = std::nullopt,
        const TokenData& data = std::monostate{}
    );
    [[nodiscard]] int peek(size_t offset = 0) const;
    [[nodiscard]] Option<SourceSpan> get_view(std::size_t len) const;
    void consume(size_t n = 1);

    [[nodiscard]] std::size_t get_digit_len() const;
    [[nodiscard]] std::size_t get_whitespace_len() const;
    [[nodiscard]] std::size_t get_identifier_len() const;
    [[nodiscard]] std::size_t get_comment_len() const;

    [[nodiscard]] static TokenType get_keyword(std::string_view name);
    [[nodiscard]] static bool is_identifier_start(const int chr) {
        return std::isalpha(chr) != 0;
    }
    [[nodiscard]] static bool is_identifier_char(const int chr) {
        return std::isalnum(chr) != 0 || chr == '_';
    }
    [[nodiscard]] static bool is_whitespace_char(const int chr) {
        return chr == ' ' || chr == '\t';
    }
    [[nodiscard]] static bool is_digit(const int chr) { return std::isdigit(chr) != 0; }

    std::vector<Token> m_tokens;
    const SourceFile& m_source_file;
    SourceSpan m_remaining{};
    int m_line{1};
    int m_column{0};
    CompilationContext& m_ctx;
};

std::vector<Token> lex(const SourceFile& input, CompilationContext& ctx);
}  // namespace lxr