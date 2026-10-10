#pragma once

#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "atom.hpp"
#include "fmt/base.h"
#include "meta.hpp"

namespace type {

struct TypeID {
    TypeID() = delete;
    constexpr explicit operator size_t() const noexcept { return id; }

    [[nodiscard]] std::string get_name() const;
    constexpr auto operator<=>(const TypeID& underlying) const noexcept = default;

private:
    size_t id{};

    friend class TypeRegistry;
    explicit constexpr TypeID(const size_t id)
        : id(id) {}

public:
    static const TypeID InvalidID;
};

constexpr TypeID TypeID::InvalidID = TypeID(std::numeric_limits<size_t>::max());

enum class BuiltinType : std::uint8_t {
    I64,
    I32,
    I16,
    I8,
    BOOLEAN,
    UNIT,
    INFERRED,
};

namespace ast {
using Type = std::variant<BuiltinType, AtomIdentifier>;
}

struct Function {
    AtomIdentifier identifier{};
    std::vector<TypeID> parameters{};

    [[nodiscard]] std::string to_string() const;
};

struct StructField {
    AtomIdentifier identifier{};
    TypeID type;

    [[nodiscard]] std::string to_string() const;
};

struct Struct {
    AtomIdentifier identifier{};
    std::optional<std::vector<StructField>> fields{};

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] bool is_opaque() const { return this->fields.has_value(); }
};

struct Alias {
    AtomIdentifier identifier{};
    TypeID underlying;

    [[nodiscard]] std::string to_string() const;
};

struct Pointer {
    AtomIdentifier identifier{};
    TypeID underlying;

    [[nodiscard]] std::string to_string() const;
};

using TypeVariant = std::variant<BuiltinType, Function, Struct, Alias, Pointer>;
template <typename T>
concept TypeVariantMember = meta::variant_member<TypeVariant, T>;

struct Type : TypeVariant {
    [[nodiscard]] bool same_as(const Type& other) const;
    bool operator==(const Type& other) const noexcept { return this->same_as(other); }

    [[nodiscard]] std::string to_string() const;

private:
    [[nodiscard]] bool same_as(BuiltinType t) const;
    [[nodiscard]] bool same_as(const Function& f) const;
    [[nodiscard]] bool same_as(const Struct& s) const;
    [[nodiscard]] bool same_as(const Alias& a) const;
    [[nodiscard]] bool same_as(const Pointer& p) const;
};

class TypeRegistry {
public:
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    TypeRegistry(TypeRegistry&&) = delete;
    TypeRegistry& operator=(TypeRegistry&&) = delete;

    static TypeRegistry& instance();
    void register_type(Type type);
    [[nodiscard]] const Type& get_type(TypeID id) const;
    [[nodiscard]] TypeID get_type_id(const Type& type) const;
    [[nodiscard]] TypeID get_type_id(std::string_view name) const;
    [[nodiscard]] TypeID get_type_id(const ast::Type& type) const;
    [[nodiscard]] std::vector<TypeID> type_ids() const;

private:
    TypeRegistry();

    std::deque<Type> types;
};

TypeID i8() noexcept;
TypeID i16() noexcept;
TypeID i32() noexcept;
TypeID i64() noexcept;

TypeID boolean() noexcept;

TypeID unit() noexcept;
TypeID inferred() noexcept;

template <TypeVariantMember Type>
std::string to_string(Type&& type) {
    return type.to_string();
}
std::string to_string(BuiltinType type);
std::string to_string(TypeID id);

}  // namespace type

template <>
struct fmt::formatter<type::TypeID> : formatter<string_view> {
    auto format(const type::TypeID& type_id, format_context& ctx) const {
        const string_view name = type_id.get_name();
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<type::BuiltinType> : formatter<string_view> {
    auto format(const type::BuiltinType& type, format_context& ctx) const {
        const string_view name = type::to_string(type);
        return formatter<string_view>::format(name, ctx);
    }
};