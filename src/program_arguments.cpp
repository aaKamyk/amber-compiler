#include "program_arguments.hpp"

#include <cstring>
#include <fstream>
#include <memory>

#include "fmt/core.h"

Arguments parseArguments(const std::span<const char*> args) {
    if (args.size() < 2) {
        throw std::runtime_error("[Arguments Parser] Usage: act <input_path>");
    }
    Arguments parsed_args = {
        .input_path = fs::path(args[1]),
        .output_path = fs::path(args[1]).replace_extension("ll")
    };

    if (!fs::exists(parsed_args.input_path)) {
        throw std::runtime_error(
            fmt::format(
                "[Arguments Parser] Input path: {} does not exist",
                parsed_args.input_path.string()
            )
        );
    }

    if (args.size() > 2) {
        if (strcmp(args[2], "-o") != 0) {
            throw std::runtime_error(
                "[Arguments Parser] Unknown option: expected \'-o\'"
            );
        }
        if (args.size() < 4) {
            throw std::runtime_error(
                "[Arguments Parser] Must provide output path after \'-o\'"
            );
        }
        parsed_args.output_path = fs::path(args[3]);
    }
    return parsed_args;
}

std::unique_ptr<std::istream> getInputFromFile(const Arguments& args) {
    auto file =
        std::make_unique<std::ifstream>(args.input_path, std::ios::in | std::ios::binary);
    if (!file->is_open()) {
        throw std::runtime_error(
            fmt::format(
                "[Arguments Parser] Could not open input file: {}",
                args.input_path.string()
            )
        );
    }
    return file;
}

std::unique_ptr<std::ostream> getOutputFromFile(const Arguments& args) {
    auto file = std::make_unique<std::ofstream>(args.output_path);

    if (!file->is_open()) {
        throw std::runtime_error(
            fmt::format(
                "[Arguments Parser] Could not open output file: {}",
                args.output_path.string()
            )
        );
    }

    return file;
}