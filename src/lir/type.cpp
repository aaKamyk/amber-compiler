#include "type.hpp"

#include <map>
#include <stdexcept>

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
    /// TODO add other types
    if (!mapping.contains(type)) { throw std::runtime_error("Non builtin type"); }
    return Type{mapping.at(type)};
}

constexpr bool lir::Type::operator==(const Type& other) {
    if (this->data.index() != other.data.index()) { return false; }

    if (this->is<PrimitiveType>() && other.is<PrimitiveType>()) {
        return this->get<PrimitiveType>() == other.get<PrimitiveType>();
    }

    if (this->is<PointerType>() && other.is<PointerType>()) {
        return this->get<PointerType>().underlying == other.get<PointerType>().underlying;
    }

    if (this->is<StructureType>() && other.is<StructureType>()) {
        return this->get<StructureType>().identifier ==
               other.get<StructureType>().identifier;
    }

    return false;
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
