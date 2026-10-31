#include "source_file.hpp"

#include <fstream>

#include "fmt/base.h"

using namespace lib;

Option<SourceFile> SourceFile::from_path(const fs::path& path) {
    auto storage = SourceFile::load_file(path);
    if (!storage.buffer) { return option::None; }
    return Option{SourceFile{std::move(storage)}};
}

Option<std::span<const std::byte>> SourceFile::bytes() const {
    if (!this->storage.buffer) { return option::None; }
std::span<const std::byte> SourceFile::bytes() const {
    return std::span<const std::byte>{this->storage.buffer.get(), this->storage.size};
}
    return option::some(
        std::span<const std::byte>{this->storage.buffer.get(), this->storage.size}
    );
}

SourceFile::SourceFile(FixedBuffer<std::byte> buffer)
    : storage(std::move(buffer)) {}

FixedBuffer<std::byte> SourceFile::load_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
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