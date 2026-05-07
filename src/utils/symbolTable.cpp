#include "symbolTable.hpp"

#include <algorithm>
#include <ranges>

NameID SymbolTable::get_name_id(const std::string_view symbol) const {
    if (const auto it = name_to_nid.find(symbol.data()); it != name_to_nid.end()) {
        return it->second;
    }
    std::abort();
}

NameID SymbolTable::add_name(const std::string& symbol) {
    auto [it, _] = name_to_nid.try_emplace(symbol, NameID::next());
    return it->second;
}

bool SymbolTable::has_name(const std::string_view symbol) const {
    return name_to_nid.contains(symbol.data());
}

SymbolMetadata SymbolTable::get_metadata(const SymbolID sid) const {
    if (const auto it = id_to_info.find(sid); it != id_to_info.end()) {
        return it->second;
    }
    std::abort();
}

SymbolID SymbolTable::add_symbol(const SymbolMetadata& data) {
    auto [it, _] = id_to_info.try_emplace(SymbolID::next(), data);
    return it->first;
}

bool SymbolTable::has_symbol(const SymbolMetadata& data) const {
    return std::ranges::any_of(id_to_info, [data](auto&& entry) -> bool {
        return entry.second == data;
    });
}