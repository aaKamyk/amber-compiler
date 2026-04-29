#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class SymbolTable {
public:
    uint64_t add_symbol(const std::string& symbol);
    std::string_view get_symbol_name(uint64_t id) const;
    uint64_t get_symbol_id(std::string_view symbol) const;
    bool has_symbol(std::string_view symbol) const;

private:
    uint64_t last_id{};
    std::unordered_map<uint64_t, std::string> id_to_symbol;
    std::unordered_map<std::string, uint64_t> symbol_to_id;
};
