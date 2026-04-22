#include <iostream>

#include "program_arguments.hpp"

int main(const int argc, const char** argv) {
    try {
        const auto args = parseArguments(std::span(argv, static_cast<size_t>(argc)));
        std::cout << args.input_path << '\n';
        std::cout << args.output_path << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}