add_executable(kcp
        main.cpp
        program_arguments.cpp
)

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(hir)
add_subdirectory(utils)
add_subdirectory(lir)