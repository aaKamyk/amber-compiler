#pragma once

#include <memory>

template <typename To, typename From>
std::unique_ptr<To> dynamic_unique_cast(std::unique_ptr<From>& ptr) {
    if (dynamic_cast<To*>(ptr.get())) {
        return std::unique_ptr<To>(static_cast<To*>(ptr.release()));
    }
    return nullptr;
}