#include "type.hpp"

#include <map>
#include <stdexcept>

std::string TypeID::get_name() const {
    const auto& [type] = TypeRegistry::instance().get_type(*this);
    std::string_view name;
    if (const auto* t = std::get_if<BuiltinType>(&type)) { return to_string(*t); }
    if (const auto* f = std::get_if<Function>(&type)) { return f->identifier.str(); }
    if (const auto* s = std::get_if<Struct>(&type)) { return s->identifier.str(); }
    if (const auto* a = std::get_if<Alias>(&type)) { return a->identifier.str(); }
    if (const auto* p = std::get_if<Pointer>(&type)) { return p->identifier.str(); }
    throw std::logic_error("Invalid std::variant state");
}

std::string_view Function::get_name() const {
    return this->identifier.name;
}

std::string_view StructField::get_name() const {
    return this->identifier.name;
}

std::string_view Struct::get_name() const {
    return this->identifier.name;
}

std::string_view Alias::get_name() const {
    return this->identifier.name;
}

std::string_view Pointer::get_name() const {
    return this->identifier.name;
}

bool Type::same_as(const Type& other) const {
    return std::visit([this](auto&& t) { return this->same_as(t); }, other.type);
}

bool Type::same_as(const BuiltinType t) const {
    if (const auto* builtin = std::get_if<BuiltinType>(&this->type)) {
        return *builtin == t;
    }
    return false;
}

bool Type::same_as(const Function& f) const {
    if (const auto* func = std::get_if<Function>(&this->type)) {
        /// TODO function equality resolution
        return true;
    }
    return false;
}

bool Type::same_as(const Struct& s) const {
    if (const auto* struc = std::get_if<Struct>(&this->type)) {
        return struc->identifier == s.identifier;
    }
    return false;
}

bool Type::same_as(const Alias& a) const {
    if (const auto* alias = std::get_if<Alias>(&this->type)) {
        return TypeRegistry::instance().get_type(alias->underlying).same_as(a);
    }
    return this->same_as(TypeRegistry::instance().get_type(a.underlying));
}

bool Type::same_as(const Pointer& p) const {
    if (const auto* ptr = std::get_if<Pointer>(&this->type)) {
        return ptr->underlying == p.underlying;
    }
    return false;
}

TypeRegistry& TypeRegistry::instance() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::register_type(Type type) {
    if (std::ranges::find(types, type) != types.end()) {
        throw std::runtime_error("Type already registered");
    }
    types.emplace_back(std::move(type));
}

const Type& TypeRegistry::get_type(const TypeID id) const {
    if (id.id >= types.size()) { throw std::runtime_error("Type not found"); }
    return types[id.id];
}

TypeID TypeRegistry::get_type_id(const Type& type) {
    const auto it = std::ranges::find(this->types, type);
    if (it == types.end()) { throw std::runtime_error("Type not found"); }
    return TypeID(std::distance(types.begin(), it));
}

TypeRegistry::TypeRegistry() {
    /// Register builtin types
    this->register_type(Type{.type = BuiltinType::I8});
    this->register_type(Type{.type = BuiltinType::I16});
    this->register_type(Type{.type = BuiltinType::I32});
    this->register_type(Type{.type = BuiltinType::I64});

    this->register_type(Type{.type = BuiltinType::BOOL});

    this->register_type(Type{.type = BuiltinType::VOID});
    this->register_type(Type{.type = BuiltinType::AUTO});
}

TypeID types::i8() noexcept {
    static TypeID i8_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::I8});
    return i8_id;
}

TypeID types::i16() noexcept {
    static TypeID i16_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::I16});
    return i16_id;
}

TypeID types::i32() noexcept {
    static TypeID i32_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::I32});
    return i32_id;
}

TypeID types::i64() noexcept {
    static TypeID i64_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::I64});
    return i64_id;
}

TypeID types::bool_() noexcept {
    static TypeID bool_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::BOOL});
    return bool_id;
}

TypeID types::void_() noexcept {
    static TypeID void_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::VOID});
    return void_id;
}

TypeID types::auto_() noexcept {
    static TypeID auto_id =
        TypeRegistry::instance().get_type_id(Type{.type = BuiltinType::AUTO});
    return auto_id;
}

std::string to_string(const BuiltinType type) {
    // clang-format off
    static const std::map<BuiltinType, const char*> type_strings = {
        {BuiltinType::I64, "i64"},
        {BuiltinType::I32, "i32"},
        {BuiltinType::I16, "i16"},
        {BuiltinType::I8, "i8"},
        {BuiltinType::BOOL, "bool"},
        {BuiltinType::VOID, "void"},
        {BuiltinType::AUTO, "auto"},
    };
    // clang-format on
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}

std::string to_string(const TypeID id) {
    return std::string{id.get_name()};
}
