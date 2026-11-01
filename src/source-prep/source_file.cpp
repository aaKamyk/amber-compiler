#include "source_file.hpp"

#include <fstream>

using namespace lib;

Option<SourceFile> SourceFile::from_path(const fs::path& path) {
    auto storage = SourceFile::load_file(path);
    if (!storage.buffer) { return option::None; }
    auto lines = SourceFile::get_lines(storage);

    return Option{SourceFile{std::move(storage), std::move(lines)}};
}

std::span<const std::byte> SourceFile::bytes() const {
    return std::span<const std::byte>{this->storage.buffer.get(), this->storage.size};
}

Option<std::string> SourceFile::get_string(const SourceSpan span) const {
    using namespace lib;
    const auto bytes = this->bytes();

    if (span.start + span.size >= this->size()) { return option::none<std::string>(); }
    return option::some(
        std::string(reinterpret_cast<const char*>(&bytes[span.start]), span.size)
    );
}

Option<std::string_view> SourceFile::get_string_view(const SourceSpan span) const {
    using namespace lib;
    const auto bytes = this->bytes();

    if (span.start + span.size >= this->size()) {
        return option::none<std::string_view>();
    }

    return Option<std::string_view>(
        reinterpret_cast<const char*>(&bytes[span.start]), span.size
    );
}

SourceSpan SourceFile::enclosing_line(const SourceSpan span) const {
    std::size_t i = 0;
    for (; i < this->lines.size(); i++) {
        if (this->lines[i].start > span.size) { break; }
    }

    return this->lines[i - 1];
}

SourceFile::SourceFile(Slice<std::byte> buffer, std::vector<SourceSpan> lines)
    : storage(std::move(buffer)), lines(std::move(lines)) {}

Slice<std::byte> SourceFile::load_file(const fs::path& path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file.is_open()) { return {.buffer = nullptr, .size = 0ull}; }

    const auto file_size = file.tellg();
    if (file_size < 0) { return {.buffer = nullptr, .size = 0ull}; }

    const auto size = static_cast<std::size_t>(file_size);
    auto buffer = std::make_unique<std::byte[]>(size);
    file.seekg(0, std::ios::beg);
    if (!file.read(
            reinterpret_cast<std::istream::char_type*>(buffer.get()),
            static_cast<std::streamsize>(size)
        )) {
        return {.buffer = nullptr, .size = 0ull};
    }

    return {.buffer = std::move(buffer), .size = size};
}

std::vector<SourceSpan> SourceFile::get_lines(const Slice<std::byte>& buffer) {
    std::vector<SourceSpan> lines;
    const std::string_view file{
        reinterpret_cast<const char*>(buffer.buffer.get()), buffer.size
    };
    std::size_t current = 0;

    while (current < buffer.size) {
        const auto line_end = file.find('\n', current);

        // Last line without a trailing newline
        if (line_end == std::string_view::npos) {
            lines.emplace_back(current, buffer.size - current);
            break;
        }

        // Add a span [start, end] span and advance to end + 1
        lines.emplace_back(current, line_end - current);
        current = line_end + 1;
        if (current >= buffer.size) { break; }
    }

    return lines;
}