#include "context.hpp"

#include <utility>

#include "fmt/core.h"

void CompilerError::print(CompilationContext* ctx) const {
    if (!ctx) { return; }

    const auto& file = ctx->source_map.get_root_file();
    const auto line_span = file.enclosing_line(this->span);
    const auto line = file.get_string_view(line_span).expect(
        "Expected parser to provide correct source span"
    );

    fmt::println(
        "{}:{}:{} error: {}", ctx->source_map.get_root_file_path().string(),
        this->span.start, this->span.size, this->message
    );
    fmt::println("{}", line);
    fmt::println("{: <{}}{:^^{}}", "", span.start - line_span.start, "", span.size);
}

CompilerError::CompilerError(const SourceSpan span, std::string message)
    : span(span), message(std::move(message)) {}

SyntaxError::SyntaxError(
    const SourceSpan span, const std::string& message, const size_t token_num
)
    : CompilerError(span, message), token_num(token_num) {}

LogicError::LogicError(const SourceSpan span, const std::string& message)
    : CompilerError(span, message) {}