#include "symbolTable.hpp"

#include <limits>

uint64_t SymbolTable::add_symbol(const std::string& symbol) {
    if (const auto it = symbol_to_id.find(symbol); it != symbol_to_id.end()) { return it->second; }
    symbol_to_id[symbol] = last_id;
    id_to_symbol[last_id++] = symbol;
    return last_id;
}

std::string_view SymbolTable::get_symbol_name(const uint64_t id) const {
    if (const auto it = id_to_symbol.find(id); it != id_to_symbol.end()) { return it->second; }
    return {};
}
uint64_t SymbolTable::get_symbol_id(const std::string_view symbol) const {
    if (const auto it = symbol_to_id.find(symbol.data()); it != symbol_to_id.end()) {
        return it->second;
    }
    return std::numeric_limits<uint64_t>::max();
}

bool SymbolTable::has_symbol(const std::string_view symbol) const {
    return symbol_to_id.contains(symbol.data());
}