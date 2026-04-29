cmake_minimum_required(VERSION 4.0)

project(kcp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(kcp
        src/main.cpp
        src/program_arguments.cpp

        src/lexer/lexer.cpp

        src/parser/parser.cpp
        src/parser/ast.cpp
        src/parser/symbolTable.cpp
        
        src/hir/hir.cpp
)

target_include_directories(kcp PRIVATE
        src/
)
