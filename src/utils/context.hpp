#pragma once

#include <filesystem>
#include <vector>

class CompilationContext;

struct CompilerError {
    int line;
    int column;
    std::string message;

    virtual ~CompilerError() = default;
    virtual void print(CompilationContext* ctx) const;
    CompilerError(int line, int column, std::string message);
};

struct SyntaxError final : CompilerError {
    size_t token_num{};

    // void print(CompilationContext* ctx) const override;
    SyntaxError(int line, int column, const std::string& message, size_t token_num);
};

struct LogicError final : CompilerError {
    // void print(CompilationContext* ctx) const override;
    LogicError(int line, int column, const std::string& message);
};

class CompilationContext {
public:
    [[nodiscard]] std::string get_line(std::size_t line_num) const;
    [[nodiscard]] std::string get_path() const;
    [[nodiscard]] auto& syntax_errors() { return m_syntax_errors; }
    [[nodiscard]] auto& logic_errors() { return m_logic_errors; }

    void new_line(std::streampos offset);
    void set_path(const std::filesystem::path& path);
    void set_stream(std::istream& stream);

private:
    std::filesystem::path input_path{};
    std::istream* input_stream{nullptr};
    std::vector<std::streampos> line_offsets{0};

    std::vector<SyntaxError> m_syntax_errors{};
    std::vector<LogicError> m_logic_errors{};
};