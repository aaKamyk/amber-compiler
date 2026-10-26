#pragma once

#include "utils/util.hpp"

struct ReturnCode {
    int value{0};
};

inline constexpr auto SUCCESS_CODE = ReturnCode{.value = 0};
inline constexpr auto FAILURE_CODE = ReturnCode{.value = -1};

Result<ReturnCode> run_compiler(int argc, const char** argv);