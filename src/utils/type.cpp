#include "type.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>

std::ostream& operator<<(std::ostream& os, const Type type) {
    static const std::unordered_map<Type, const char*> type_strings = {
        {Type::INT, "int"}, {Type::VOID, "void"}
    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return os << type_strings.at(type);
}