#include <iostream>

#include "args.hpp"
#include "hir/hir.hpp"
#include "lexer/lexer.hpp"
#include "lir/lir.hpp"
#include "parser/parser.hpp"

void run_compiler(const int argc, const char** argv) {
    const auto args = parse_arguments(std::span(argv, static_cast<size_t>(argc)));
    const auto input = get_input_from_file(args);
    const auto output = get_output_from_file(args);

    CompilationContext ctx;
    ctx.set_path(args.input_path);
    ctx.set_stream(*input);

    auto tokens = lxr::lex(*input, ctx);
    auto ast = ast::parse(std::move(tokens), ctx);
    auto hir = hir::translate(std::move(ast), ctx);
    hir::print_program(hir, std::cout);
    const auto lir = lir::translate(std::move(hir), ctx);

    lir::print_program(lir, *output);
    fmt::println("Output written to: {}", args.output_path.string());
}