#pragma once

#include "driver/args.hpp"
#include "source_file.hpp"

class SourceMap {
public:
    static Option<SourceMap> from_args(const Arguments& args);
    bool insert_new(const fs::path& path);

private:
    using FileSpanTuple = std::pair<SourceFile, SourceSpan>;

    std::vector<FileSpanTuple> source_files{};
};
