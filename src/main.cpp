#include <iostream>

#include "hir/hir.hpp"
#include "lexer/lexer.hpp"
#include "parser/node.hpp"
#include "parser/parser.hpp"
#include "program_arguments.hpp"

void runCompiler(const int argc, const char** argv) {
    const auto args = parseArguments(std::span(argv, static_cast<size_t>(argc)));
    const auto input = getInputFromFile(args);
    Lexer lexer(*input);
    auto tokens = lexer.lex();
    Parser parser(std::move(tokens));
    auto ast = parser.parse();
    print_program(ast);
    hir::Hir translator(std::move(ast));
    auto hir = translator.translate();
    hir::print_program(hir);
}

int main(const int argc, const char** argv) {
    try {
        runCompiler(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}