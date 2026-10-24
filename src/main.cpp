#include <iostream>

#include "driver/driver.hpp"

int main(const int argc, const char** argv) {
    try {
        run_compiler(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}