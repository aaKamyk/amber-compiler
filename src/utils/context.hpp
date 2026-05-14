#pragma once

#include <filesystem>
#include <vector>

struct ParserError {
    int line;
    int column;
    std::string message;

    size_t token_num;

    void print() const;
};

class CompilationContext {
public:
    void new_line(std::streampos offset);
    [[nodiscard]] std::string get_line(std::size_t line_num) const;
    [[nodiscard]] std::string get_path() const;
    [[nodiscard]] auto& parser_errors() { return m_parser_errors; }

    void set_path(const std::filesystem::path& path);
    void set_stream(std::istream& stream);

private:
    std::filesystem::path input_path{};
    std::istream* input_stream{nullptr};
    std::vector<std::streampos> line_offsets{0};
    std::vector<ParserError> m_parser_errors{};
};