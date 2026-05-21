#include <iostream>

#include "hir/hir.hpp"
#include "lexer/lexer.hpp"
#include "lir/lir.hpp"
#include "parser/parser.hpp"
#include "program_arguments.hpp"

void runCompiler(const int argc, const char** argv) {
    const auto args = parseArguments(std::span(argv, static_cast<size_t>(argc)));
    const auto input = getInputFromFile(args);
    CompilationContext ctx;
    ctx.set_path(args.input_path);
    ctx.set_stream(*input);
    auto tokens = lxr::lex(*input, ctx);
    auto ast = ast::parse(std::move(tokens), ctx);
    auto hir = hir::translate(std::move(ast), ctx);
    hir::print_program(hir);
    const auto lir = lir::translate(std::move(hir), ctx);
    lir::print_program(lir);
}

int main(const int argc, const char** argv) {
    try {
        runCompiler(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}