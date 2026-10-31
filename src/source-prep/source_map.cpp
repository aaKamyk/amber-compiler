#include "source_map.hpp"

Option<SourceMap> SourceMap::from_args(const Arguments& args) {
    using namespace lib;
    auto source_file_opt = SourceFile::from_path(args.input_path);
    if (!source_file_opt.has_some()) { return option::none<SourceMap>(); }
    auto source_file = std::move(source_file_opt).unwrap();
    auto source_span = SourceSpan{.start = 0, .size = source_file.size()};

    SourceMap source_map{};
    source_map.source_files.emplace_back(
        args.input_path, std::move(source_file), source_span
    );

    return option::some(std::move(source_map));
}

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