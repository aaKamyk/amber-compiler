#include "type.hpp"

#include <map>
#include <stdexcept>

lir::Type lir::type_from(const type::TypeID type) {
    // clang-format off
    static const std::map<type::TypeID, Type> mapping = {
        {type::i64(), Type::I64},
        {type::i32(), Type::I32},
        {type::i16(), Type::I16},
        {type::i8(), Type::I8},
        {type::i16(), Type::I1},
        {type::unit(), Type::VOID},
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
