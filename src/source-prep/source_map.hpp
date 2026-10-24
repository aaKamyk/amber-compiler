#pragma once

#include "source_file.hpp"

struct SourceSpan {
    std::size_t start;
    std::size_t size;
};

class SourceMap {
public:
    bool insert_new(const fs::path& path);

private:
    using FileSpanTuple = std::pair<SourceFile, SourceSpan>;

    std::vector<FileSpanTuple> source_files{};
};
