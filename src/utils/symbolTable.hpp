#pragma once

#include <string>
#include <unordered_map>

#include "type.hpp"

struct SymbolMetadata {
    type::TypeID type{type::unit()};
    bool is_mut{false};

    constexpr bool operator==(const SymbolMetadata& other) const noexcept = default;
};

class SymbolTable {
public:
    static std::string get_unique_name(std::string_view name);

    bool has_symbol(std::string_view symbol) const;
    void add_symbol(std::string_view name, SymbolMetadata data);
    SymbolMetadata get_metadata(std::string_view name) const;

private:
    std::unordered_map<std::string, SymbolMetadata> name_to_info;
};
