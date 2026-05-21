#include "type.hpp"

#include <iostream>
#include <map>

lir::Type lir::type_from(const ::Type type) {
    static const std::map<::Type, Type> mapping = {
        {::Type::I32, Type::I32},
        {::Type::VOID, Type::VOID},
    };
    if (!mapping.contains(type)) { throw std::runtime_error("Unknown type from hir"); }
    return mapping.at(type);
}

std::ostream& lir::operator<<(std::ostream& os, const Type type) {
    static const std::map<Type, const char*> type_strings = {
        // integer types
        {Type::I1, "i1"},
        {Type::I8, "i8"},
        {Type::I16, "i16"},
        {Type::I32, "i32"},
        {Type::I64, "i64"},
        // special
        {Type::VOID, "void"}

    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return os << type_strings.at(type);
}