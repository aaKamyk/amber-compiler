#pragma once

#include <filesystem>
#include <vector>

#include "source-prep/source_map.hpp"

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
    SourceMap source_map;

    [[nodiscard]] auto& syntax_errors() { return m_syntax_errors; }
    [[nodiscard]] auto& logic_errors() { return m_logic_errors; }

private:
    std::vector<SyntaxError> m_syntax_errors{};
    std::vector<LogicError> m_logic_errors{};
};