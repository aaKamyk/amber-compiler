#include "type.hpp"

#include <iostream>
#include <map>

std::ostream& lir::operator<<(std::ostream& os, const Type type) {
    static const std::map<Type, const char*> type_strings = {
        {Type::I32, "i32"}, {Type::VOID, "void"}
    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return os << type_strings.at(type);
}

lir::Type lir::type_from(const ::Type type) {
    static const std::map<::Type, Type> mapping = {
        {::Type::I32, Type::I32},
        {::Type::VOID, Type::VOID},
    };
    if (!mapping.contains(type)) { throw std::runtime_error("Unknown type from hir"); }
    return mapping.at(type);
}