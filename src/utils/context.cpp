#include "context.hpp"

#include <cassert>
#include <iostream>

void ParserError::print() const {
    static const std::string RED = "\033[1;31m";
    static const std::string RESET = "\033[0m";
    std::cout << this->line << ':' << this->column << " error: " << this->message << '\n';
}

void CompilationContext::new_line(const std::streampos offset) {
    line_offsets.push_back(offset);
}

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

void CompilationContext::set_path(const std::filesystem::path& path) {
    this->input_path = path;
}

void CompilationContext::set_stream(std::istream& stream) {
    this->input_stream = &stream;
}