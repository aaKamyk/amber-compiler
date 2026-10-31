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

const SourceFile& SourceMap::get_root_file() const {
    return this->source_files.front().source;
}

fs::path SourceMap::get_root_file_path() const {
    return this->source_files.front().path;
}