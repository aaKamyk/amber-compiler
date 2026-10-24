add_executable(act main.cpp)

target_include_directories(act PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(driver)
add_subdirectory(source-prep)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(hir)
add_subdirectory(utils)
add_subdirectory(lir)

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 11.0.2
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(act PRIVATE fmt)