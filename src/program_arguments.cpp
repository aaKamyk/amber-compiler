#include "program_arguments.hpp"

#include <cstring>

Arguments parseArguments(const std::span<const char*> args){
    if (args.size() < 2) {
        throw std::runtime_error("Usage: kcp <input_path>");
    }
    Arguments parsed_args = {
        .input_path = fs::path(args[1]),
    };
    if (!fs::exists(parsed_args.input_path)) {
        throw std::runtime_error("Input path does not exist");
    }
    if (args.size() > 2) {
        if (strcmp(args[2], "-o") != 0) {
            throw std::runtime_error("Unknown option: expected \'-o\'");
        }
        if (args.size() < 4) {
            throw std::runtime_error("Must provide output path after \'-o\'");
        }
        parsed_args.output_path = fs::path(args[3]);
    }
    return parsed_args;
}