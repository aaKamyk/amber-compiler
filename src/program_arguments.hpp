#pragma once

#include <filesystem>
#include <span>

namespace fs = std::filesystem;

struct Arguments {
    fs::path input_path;
    fs::path output_path{ fs::current_path() / "a.out" };
};

Arguments parseArguments(std::span<const char*> args);