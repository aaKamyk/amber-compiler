#include "type.hpp"

#include <map>
#include <stdexcept>

#include "fmt/format.h"

lir::Type lir::type_from(const type::TypeID type) {
    // clang-format off
    static const std::map<type::TypeID, PrimitiveType> mapping = {
        {type::i64(), PrimitiveType::I64},
        {type::i32(), PrimitiveType::I32},
        {type::i16(), PrimitiveType::I16},
        {type::i8(), PrimitiveType::I8},
        {type::boolean(), PrimitiveType::I1},
        {type::unit(), PrimitiveType::VOID},
    };
    // clang-format on
    if (mapping.contains(type)) { return Type{mapping.at(type)}; }

    if (const auto& full_type = type::TypeRegistry::instance().get_type(type);
        full_type.is<type::Struct>()) {
        const auto it = TypeRegistry::definitions.find(full_type.to_string());
        if (it == TypeRegistry::definitions.end()) {
            throw std::runtime_error(
                fmt::format("Unknown type: {}", full_type.to_string())
            );
        }

        return Type{.data = it->second};
    }

    return Type{.data = PrimitiveType::VOID};
}

lir::PointerType::PointerType(const PointerType& other)
    : underlying(
          other.underlying != nullptr ? std::make_unique<Type>(*other.underlying)
                                      : std::unique_ptr<Type>(nullptr)
      ) {}

lir::PointerType& lir::PointerType::operator=(const PointerType& other) {
    if (this == &other) { return *this; }

    this->underlying = other.underlying != nullptr
                           ? std::make_unique<Type>(*other.underlying)
                           : std::unique_ptr<Type>(nullptr);
    return *this;
}

lir::StructureType::StructureType(const StructureType& other)
    : identifier(other.identifier->clone()),
      fields([&other]() -> std::vector<std::unique_ptr<Type>> {
          auto copied =
              other.fields |
              std::views::transform([](const auto& field) -> std::unique_ptr<Type> {
                  return field != nullptr ? std::make_unique<Type>(*field)
                                          : std::unique_ptr<Type>(nullptr);
              });

          return {copied.begin(), copied.end()};
      }()),
      is_packed(other.is_packed) {}

lir::StructureType& lir::StructureType::operator=(const StructureType& other) {
    if (this == &other) { return *this; }

    auto copied = other.fields |
                  std::views::transform([](const auto& field) -> std::unique_ptr<Type> {
                      return field != nullptr ? std::make_unique<Type>(*field)
                                              : std::unique_ptr<Type>(nullptr);
                  });

    this->identifier = other.identifier->clone();
    this->fields = {copied.begin(), copied.end()};
    this->is_packed = other.is_packed;
    return *this;
}

lir::StructureType& lir::TypeRegistry::get_definition(std::string_view name) {
    for (auto& [strct_name, strct] : TypeRegistry::definitions) {
        if (strct_name == name) { return strct; }
    }

    throw std::runtime_error(fmt::format("Unknown type: {}", name));
}

void lir::TypeRegistry::register_struct(type::Struct struct_type) {
    std::vector<std::unique_ptr<Type>> fields{};
    if (struct_type.fields.has_value()) {
        auto transformed =
            struct_type.fields.value() |
            std::views::transform([](auto&& struct_field) -> std::unique_ptr<Type> {
                return std::make_unique<Type>(type_from(struct_field.type));
            });
        fields.assign(transformed.begin(), transformed.end());
    }

    StructureType lir_struct = {
        std::make_unique<AtomIdentifier>(struct_type.identifier), std::move(fields)
    };
}

std::string to_string(const lir::PrimitiveType type) {
    static const std::map<lir::PrimitiveType, const char*> type_strings = {
        // integer types
        {lir::PrimitiveType::I1, "i1"},
        {lir::PrimitiveType::I8, "i8"},
        {lir::PrimitiveType::I16, "i16"},
        {lir::PrimitiveType::I32, "i32"},
        {lir::PrimitiveType::I64, "i64"},
        // special
        {lir::PrimitiveType::VOID, "void"}

    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}

std::string to_string(const lir::PointerType& type) {
    return to_string(*type.underlying) + "*";
}
std::string to_string(const lir::StructureType& type) {
    return type.identifier->name;
}
std::string to_string(const lir::Type& type) {
    return std::visit(
        [](auto&& arg) -> std::string { return to_string(arg); }, type.data
    );
}
