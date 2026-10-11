#pragma once

#include <map>
#include <ranges>

#include "utils/type.hpp"

namespace lir {

struct Type;

enum class TypeKind : uint8_t {
    Primitive,
    Pointer,
    Struct,
};

enum class PrimitiveType : uint8_t {
    // integer types
    I1,
    I8,
    I16,
    I32,
    I64,
    // special
    VOID
};

Type type_from(type::TypeID);

struct PointerType {
    std::unique_ptr<Type> underlying{nullptr};

    PointerType() = default;
    PointerType(const PointerType& other);
    PointerType(PointerType&& other) noexcept = default;
    PointerType& operator=(const PointerType& other);
    PointerType& operator=(PointerType&& other) noexcept = default;
};

struct StructureType {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::vector<std::unique_ptr<Type>> fields{};
    bool is_packed{false};

    StructureType() = default;
    StructureType(
        std::unique_ptr<AtomIdentifier> identifier,
        std::vector<std::unique_ptr<Type>> fields
    )
        : identifier(std::move(identifier)), fields(std::move(fields)) {}
    StructureType(const StructureType& other);
    StructureType(StructureType&& other) noexcept = default;
    StructureType& operator=(const StructureType& other);
    StructureType& operator=(StructureType&& other) noexcept = default;
};

struct Type {
    using TypeKinds = std::variant<PrimitiveType, PointerType, StructureType>;
    TypeKinds data;

    template <meta::variant_member<TypeKinds> T>
    [[nodiscard]] constexpr T& get() {
        return std::get<T>(data);
    }

    template <meta::variant_member<TypeKinds> T>
    [[nodiscard]] constexpr const T& get() const {
        return std::get<T>(data);
    }

    template <meta::variant_member<TypeKinds> T>
    [[nodiscard]] constexpr bool is() const {
        return std::holds_alternative<T>(data);
    }

    constexpr bool operator==(const Type& other) const {
        if (this->data.index() != other.data.index()) { return false; }

        if (this->is<PrimitiveType>() && other.is<PrimitiveType>()) {
            return this->get<PrimitiveType>() == other.get<PrimitiveType>();
        }

        if (this->is<PointerType>() && other.is<PointerType>()) {
            return this->get<PointerType>().underlying ==
                   other.get<PointerType>().underlying;
        }

        if (this->is<StructureType>() && other.is<StructureType>()) {
            return this->get<StructureType>().identifier ==
                   other.get<StructureType>().identifier;
        }

        return false;
    }
};

struct TypeRegistry {
    static StructureType& get_definition(std::string_view name);

    static void register_struct(type::Struct struct_type);

    inline static std::map<std::string, StructureType> definitions{};
};  // namespace lir

}  // namespace lir

std::string to_string(lir::PrimitiveType type);
std::string to_string(const lir::PointerType& type);
std::string to_string(const lir::StructureType& type);
std::string to_string(const lir::Type& type);

template <>
struct fmt::formatter<lir::PrimitiveType> : formatter<string_view> {
    auto format(const lir::PrimitiveType type, format_context& ctx) const {
        const string_view name = ::to_string(type);
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<lir::Type> : formatter<string_view> {
    auto format(const lir::Type& type, format_context& ctx) const {
        const string_view name = ::to_string(type);
        return formatter<string_view>::format(name, ctx);
    }
};