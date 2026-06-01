#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <variant>
#include <vector>

#include "atom.hpp"
#include "fmt/base.h"
#include "type.hpp"

template <typename... T>
struct overloaded : T... {
    using T::operator()...;
};
template <typename... T>
overloaded(T...) -> overloaded<T...>;

enum class BuiltinType : std::uint8_t {
    I64,
    I32,
    I16,
    I8,
    BOOL,
    VOID,
    AUTO,
};

struct TypeID {
    TypeID() = delete;
    constexpr explicit operator size_t() const noexcept { return id; }

    [[nodiscard]] std::string get_name() const;
    constexpr auto operator<=>(const TypeID& underlying) const noexcept = default;

private:
    size_t id{};

    friend class TypeRegistry;
    explicit TypeID(const size_t id)
        : id(id) {}
};

struct Function {
    AtomIdentifier identifier{};
    std::vector<TypeID> parameters{};

    std::string_view get_name() const;
};

struct StructField {
    AtomIdentifier identifier{};
    TypeID type;

    std::string_view get_name() const;
};

struct Struct {
    AtomIdentifier identifier{};
    std::vector<StructField> fields{};

    std::string_view get_name() const;
};

struct Alias {
    AtomIdentifier identifier{};
    TypeID underlying;

    std::string_view get_name() const;
};

struct Pointer {
    AtomIdentifier identifier{};
    TypeID underlying;

    std::string_view get_name() const;
};

struct Type {
    std::variant<BuiltinType, Function, Struct, Alias, Pointer> type{};

    [[nodiscard]] bool same_as(const Type& other) const;
    bool operator==(const Type& other) const noexcept { return this->same_as(other); }

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

    static TypeRegistry& instance();
    void register_type(Type type);
    [[nodiscard]] const Type& get_type(TypeID id) const;
    [[nodiscard]] TypeID get_type_id(const Type& type);

private:
    TypeRegistry();

    std::deque<Type> types;
};

namespace types {
TypeID i8() noexcept;
TypeID i16() noexcept;
TypeID i32() noexcept;
TypeID i64() noexcept;

TypeID bool_() noexcept;

TypeID void_() noexcept;
TypeID auto_() noexcept;
}  // namespace types

std::string to_string(BuiltinType type);
std::string to_string(TypeID id);

template <>
struct fmt::formatter<TypeID> : formatter<string_view> {
    auto format(const TypeID& type_id, format_context& ctx) const {
        const string_view name = type_id.get_name();
        return formatter<string_view>::format(name, ctx);
    }
};