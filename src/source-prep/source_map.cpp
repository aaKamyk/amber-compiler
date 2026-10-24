#include "source_map.hpp"

bool SourceMap::insert_new(const fs::path& path) {
    auto file_opt = SourceFile::from_path(path);
    if (!file_opt.has_some()) { return false; }
    auto file = std::move(file_opt).unwrap();

    const std::size_t start =
        this->source_files.empty()
            ? 0
            : this->source_files.at(this->source_files.size() - 1).first.size();

    auto span = SourceSpan{.start = start, .size = file.size()};
    this->source_files.emplace_back(std::move(file), span);
    return true;
}