#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "type.hpp"

struct SymbolID {
    uint64_t id{};

    static SymbolID next() {
        static uint64_t last_id{0};
        return {last_id++};
    }

    constexpr bool operator==(const SymbolID& other) const noexcept = default;
};

struct NameID {
    uint64_t id{};

    static NameID next() {
        static uint64_t last_id{0};
        return {last_id++};
    }
    constexpr bool operator==(const NameID& nid) const noexcept = default;
};

template <>
struct std::hash<SymbolID> {
    size_t operator()(const SymbolID& sym_id) const noexcept {
        return std::hash<uint64_t>{}(sym_id.id);
    }
};

template <>
struct std::hash<NameID> {
    size_t operator()(const NameID& name_id) const noexcept {
        return std::hash<uint64_t>{}(name_id.id);
    }
};

struct SymbolMetadata {
    Type type{};
    NameID nid{};
    uint64_t depth{};

    constexpr bool operator==(const SymbolMetadata& other) const noexcept = default;
};

class SymbolTable {
public:
    NameID get_name_id(std::string_view symbol) const;
    NameID add_name(const std::string& symbol);
    bool has_name(std::string_view symbol) const;
    SymbolMetadata get_metadata(SymbolID sid) const;
    SymbolID add_symbol(const SymbolMetadata& data);
    bool has_symbol(const SymbolMetadata& data) const;

private:
    std::unordered_map<std::string, NameID> name_to_nid;
    std::unordered_map<SymbolID, SymbolMetadata> id_to_info;
};
