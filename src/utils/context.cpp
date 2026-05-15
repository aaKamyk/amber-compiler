#include "context.hpp"

#include <cassert>
#include <iostream>
#include <utility>

void CompilerError::print(CompilationContext* ctx) const {
    if (!ctx) { return; }
    static const std::string RED = "\033[1;31m";
    static const std::string RESET = "\033[0m";

    std::cout << ctx->get_path() << ':' << this->line << ':' << this->column;
    std::cout << " error: " << this->message << '\n';
    std::cout << ctx->get_line(this->line) << '\n';
    std::cout << std::string(this->column, ' ') << "^\n";
}

CompilerError::CompilerError(const int line, const int column, std::string message)
    : line(line), column(column), message(std::move(message)) {}

SyntaxError::SyntaxError(
    const int line, const int column, const std::string& message, const size_t token_num
)
    : CompilerError(line, column, message), token_num(token_num) {}

LogicError::LogicError(const int line, const int column, const std::string& message)
    : CompilerError(line, column, message) {}

std::string CompilationContext::get_line(const std::size_t line_num) const {
    assert(input_stream);
    if (line_num == 0 || line_num > line_offsets.size()) {
        throw std::invalid_argument("Invalid line number");
    }
    input_stream->clear();
    input_stream->seekg(line_offsets[line_num - 1], std::ios::beg);
    std::string line;
    std::getline(*input_stream, line);
    return line;
}

std::string CompilationContext::get_path() const {
    return this->input_path.string();
}

void CompilationContext::new_line(const std::streampos offset) {
    line_offsets.push_back(offset);
}

void CompilationContext::set_path(const std::filesystem::path& path) {
    this->input_path = path;
}

void CompilationContext::set_stream(std::istream& stream) {
    this->input_stream = &stream;
}