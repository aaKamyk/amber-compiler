#include "source_file.hpp"

#include <fstream>

using namespace lib;

Option<SourceFile> SourceFile::from_path(const fs::path& path) {
    auto storage = SourceFile::load_file(path);
    if (!storage.buffer) { return option::None; }
    return Option{SourceFile{std::move(storage)}};
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

SourceFile::SourceFile(FixedBuffer<std::byte> buffer)
    : storage(std::move(buffer)) {}

FixedBuffer<std::byte> SourceFile::load_file(const fs::path& path) {
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