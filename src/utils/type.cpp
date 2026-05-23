#include "type.hpp"

#include <map>
#include <stdexcept>

std::string to_string(const Type type) {
    // clang-format off
    static const std::map<Type, const char*> type_strings = {
        {Type::I64, "i64"},
        {Type::I32, "i32"},
        {Type::I16, "i16"},
        {Type::I8, "i8"},
        {Type::BOOL, "bool"},
        {Type::VOID, "void"}
    };
    // clang-format on
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}
