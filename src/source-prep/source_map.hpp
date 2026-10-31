#pragma once

#include "driver/args.hpp"
#include "source_file.hpp"

class SourceMap {
public:
    struct FileData {
        fs::path path;
        SourceFile source;
        SourceSpan span;
    };

    static Option<SourceMap> from_args(const Arguments& args);

    [[nodiscard]] const SourceFile& get_root_file() const;
    [[nodiscard]] fs::path get_root_file_path() const;

private:
    std::vector<FileData> source_files{};
};
