#include <iostream>

#include "lexer/lexer.hpp"
#include "program_arguments.hpp"

void runCompiler(const int argc, const char** argv) {
    const auto args = parseArguments(std::span(argv, static_cast<size_t>(argc)));
    const auto input = getInputFromFile(args);
    Lexer lexer(*input);
    const auto tokens = lexer.lex();
    Lexer::pretty_print_tokens(tokens);
}

int main(const int argc, const char** argv) {
    try {
        runCompiler(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}