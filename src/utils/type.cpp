#include "type.hpp"

#include <stdexcept>
#include <unordered_map>

std::string to_string(const Type type) {
    static const std::unordered_map<Type, const char*> type_strings = {
        {Type::BOOL, "bool"}, {Type::I32, "i32"}, {Type::VOID, "void"}
    };
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}
std::ostream& operator<<(std::ostream& os, const Type type) {
    return os << to_string(type);
}