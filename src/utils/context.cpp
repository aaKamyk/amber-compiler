#include "context.hpp"

#include <utility>

#include "fmt/core.h"

void CompilerError::print(CompilationContext* ctx) const {
    if (!ctx) { return; }

    fmt::println(
        "{}:{}:{} error: {}", ctx->source_map.get_root_file_path().string(), this->line,
        this->column, this->message
    );
    // fmt::println("{}", ctx->get_line(this->line));
    fmt::println("{: <{}}^", "", this->column);
}

CompilerError::CompilerError(const int line, const int column, std::string message)
    : line(line), column(column), message(std::move(message)) {}

SyntaxError::SyntaxError(
    const int line, const int column, const std::string& message, const size_t token_num
)
    : CompilerError(line, column, message), token_num(token_num) {}

LogicError::LogicError(const int line, const int column, const std::string& message)
    : CompilerError(line, column, message) {}