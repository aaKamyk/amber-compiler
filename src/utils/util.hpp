#pragma once

#include <memory>

#include "lib/result.hpp"

struct Error {
    std::string message;
};

template <typename T>
using Result = cpp_result::Result<T, Error>;

template <typename To, typename From>
std::unique_ptr<To> dynamic_unique_cast(std::unique_ptr<From>& ptr) {
    if (dynamic_cast<To*>(ptr.get())) {
        return std::unique_ptr<To>(static_cast<To*>(ptr.release()));
    }
    return nullptr;
}