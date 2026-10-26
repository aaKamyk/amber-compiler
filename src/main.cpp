#include <iostream>

#include "driver/driver.hpp"

int main(const int argc, const char** argv) {
    const auto res = run_compiler(argc, argv);
    return res.unwrap_or_else([&res]() -> int {
        std::cerr << res.err()->message << std::endl;
        return -1;
    });
}