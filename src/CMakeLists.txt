add_executable(kcp
        main.cpp
        program_arguments.cpp
)

target_include_directories(kcp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(hir)
add_subdirectory(utils)
add_subdirectory(lir)