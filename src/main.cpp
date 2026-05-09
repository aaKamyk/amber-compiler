#include <iostream>

#include "hir/hir.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "program_arguments.hpp"

void runCompiler(const int argc, const char** argv) {
    const auto args = parseArguments(std::span(argv, static_cast<size_t>(argc)));
    const auto input = getInputFromFile(args);
    auto tokens = lxr::lex(*input);
    auto ast = ast::parse(std::move(tokens));
    auto hir = hir::translate(std::move(ast));
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