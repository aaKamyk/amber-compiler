#pragma once

#include <filesystem>
#include <span>

#include "lib/option.hpp"

namespace fs = std::filesystem;

using lib::option::Option;

template <typename T>
struct FixedBuffer {
    std::unique_ptr<T[]> buffer;
    std::size_t size{0ull};
};

class SourceFile {
public:
    static Option<SourceFile> from_path(const fs::path& path);

    SourceFile(SourceFile&& other) = default;
    SourceFile(const SourceFile&) = delete;
    SourceFile() = delete;

    [[nodiscard]] Option<std::span<const std::byte>> bytes() const;
    [[nodiscard]] std::size_t size() const { return storage.size; }

private:
    explicit SourceFile(FixedBuffer<std::byte> buffer);

    static FixedBuffer<std::byte> load_file(const fs::path& path);

    FixedBuffer<std::byte> storage;
};
