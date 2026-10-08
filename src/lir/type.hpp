#pragma once

#include <map>

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
    PointerType(const PointerType& other)
        : underlying(std::make_unique<Type>(*other.underlying)) {}
    PointerType(PointerType&& other) noexcept = default;
    PointerType& operator=(const PointerType& other) {
        if (this == &other) { return *this; }

        this->underlying = std::make_unique<Type>(*other.underlying);
        return *this;
    };
    PointerType& operator=(PointerType&& other) noexcept = default;
};

struct StructureType {
    std::unique_ptr<AtomIdentifier> identifier{nullptr};
    std::vector<PrimitiveType> fields{};
    bool is_packed{false};

    StructureType() = default;
    StructureType(const StructureType& other)
        : identifier{other.identifier->clone()}, fields{other.fields} {}
    StructureType(StructureType&& other) noexcept = default;
    StructureType& operator=(const StructureType& other) {
        if (this == &other) { return *this; }

        this->identifier = other.identifier->clone();
        this->fields = other.fields;
        return *this;
    }
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

    constexpr bool operator==(const Type& other);
};

class TypeRegistry {
public:
    static StructureType& get_definition(std::string_view name);

private:
    static std::map<std::string, StructureType> definitions;
};

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