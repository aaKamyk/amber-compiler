#include "type.hpp"

#include <map>
#include <stdexcept>

lir::Type lir::type_from(const TypeID type) {
    // clang-format off
    static const std::map<::TypeID, Type> mapping = {
        {types::i64(), Type::I64},
        {types::i32(), Type::I32},
        {types::i16(), Type::I16},
        {types::i8(), Type::I8},
        {types::i16(), Type::I1},
        {types::void_(), Type::VOID},
    };
    // clang-format on
    /// TODO add other types
    if (!mapping.contains(type)) { throw std::runtime_error("Non builtin type"); }
    return mapping.at(type);
}

std::string to_string(const lir::Type type) {
    static const std::map<lir::Type, const char*> type_strings = {
        // integer types
        {lir::Type::I1, "i1"},
        {lir::Type::I8, "i8"},
        {lir::Type::I16, "i16"},
        {lir::Type::I32, "i32"},
        {lir::Type::I64, "i64"},
        // special
        {lir::Type::VOID, "void"}

    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}
