#pragma once

#include "source_file.hpp"

class SourceMap {
public:
    bool insert_new(const fs::path& path);

private:
    using FileSpanTuple = std::pair<SourceFile, SourceSpan>;

    std::vector<FileSpanTuple> source_files{};
};
