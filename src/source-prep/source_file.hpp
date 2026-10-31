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

struct SourceSpan {
    std::size_t start;
    std::size_t size;

    constexpr void advance(const std::size_t n) { this->start += n; }
};

class SourceFile {
public:
    static Option<SourceFile> from_path(const fs::path& path);

    SourceFile(SourceFile&& other) = default;
    SourceFile(const SourceFile&) = delete;
    SourceFile() = delete;

    [[nodiscard]] std::span<const std::byte> bytes() const;
    [[nodiscard]] std::size_t size() const { return storage.size; }
    [[nodiscard]] Option<std::string> get_string(SourceSpan span) const;
    [[nodiscard]] Option<std::string_view> get_string_view(SourceSpan span) const;

private:
    explicit SourceFile(FixedBuffer<std::byte> buffer);

    static FixedBuffer<std::byte> load_file(const fs::path& path);

    FixedBuffer<std::byte> storage;
};
