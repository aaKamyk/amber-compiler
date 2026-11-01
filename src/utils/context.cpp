#include "context.hpp"

#include <utility>

#include "fmt/core.h"

void CompilerError::print(CompilationContext* ctx) const {
    if (!ctx) { return; }

    fmt::println(
        "{}:{}:{} error: {}", ctx->source_map.get_root_file_path().string(),
        this->span.size, this->span.size, this->message
    );
    // fmt::println("{}", ctx->get_line(this->line));
    // TODO
    fmt::println("{: <{}}^", "", 0);
}

CompilerError::CompilerError(const SourceSpan span, std::string message)
    : span(span), message(std::move(message)) {}

SyntaxError::SyntaxError(
    const SourceSpan span, const std::string& message, const size_t token_num
)
    : CompilerError(span, message), token_num(token_num) {}

LogicError::LogicError(const SourceSpan span, const std::string& message)
    : CompilerError(span, message) {}