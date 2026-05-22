#include "symbolTable.hpp"

#include <algorithm>
#include <map>
#include <ranges>

bool SymbolTable::has_symbol(const std::string_view symbol) const {
    return name_to_info.contains(symbol.data());
}

void SymbolTable::add_symbol(const std::string_view name, SymbolMetadata data) {
    this->name_to_info.insert({name.data(), data});
}

std::string SymbolTable::get_unique_name(const std::string_view name) {
    static std::map<std::string, size_t> usages;
    return std::string(name) + '.' + std::to_string(usages[name.data()]++);
}

SymbolMetadata SymbolTable::get_metadata(const std::string_view name) const {
    if (const auto it = name_to_info.find(name.data()); it != name_to_info.end()) {
        return it->second;
    }
    throw std::runtime_error("Unknown symbol");
}