#pragma once

#include <filesystem>
#include <span>

namespace fs = std::filesystem;

struct Arguments {
    fs::path input_path;
    fs::path output_path{fs::current_path() / "a.out"};
};

Arguments parse_arguments(std::span<const char*> args);
std::unique_ptr<std::istream> get_input_from_file(const Arguments& args);
std::unique_ptr<std::ostream> get_output_from_file(const Arguments& args);