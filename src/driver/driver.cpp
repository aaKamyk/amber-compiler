#include "driver/driver.hpp"

#include "args.hpp"
#include "hir/hir.hpp"
#include "lexer/lexer.hpp"
#include "lib/result.hpp"
#include "lir/lir.hpp"
#include "parser/parser.hpp"
#include "source-prep/source_map.hpp"

Result<ReturnCode> run_compiler(const int argc, const char** argv) {
    try {
        const auto args = parse_arguments(std::span(argv, static_cast<size_t>(argc)));
        auto source_map = SourceMap::from_args(args);
        CompilationContext ctx;

        if (!source_map.has_some()) {
            return Result<ReturnCode>::Err({"Could not initialize source map"});
        }
        ctx.source_map = std::move(source_map).unwrap();
        const auto output = get_output_from_file(args);

        auto tokens = lxr::lex(ctx.source_map.get_root_file(), ctx);
        auto ast = ast::parse(std::move(tokens), ctx);
        auto hir = hir::translate(std::move(ast), ctx);
        const auto lir = lir::translate(std::move(hir), ctx);

        lir::print_program(lir, *output);
        fmt::println("Output written to: {}", args.output_path.string());

    } catch (const std::exception& ex) { return Result<ReturnCode>::Err({ex.what()}); }

    return Result<ReturnCode>::Ok(SUCCESS_CODE);
}