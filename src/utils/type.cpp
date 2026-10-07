#include "type.hpp"

#include <map>
#include <stdexcept>

std::string type::TypeID::get_name() const {
    const auto& type = TypeRegistry::instance().get_type(*this);
    if (const auto* t = std::get_if<BuiltinType>(&type)) { return to_string(*t); }
    if (const auto* f = std::get_if<Function>(&type)) { return f->identifier.str(); }
    if (const auto* s = std::get_if<Struct>(&type)) { return s->identifier.str(); }
    if (const auto* a = std::get_if<Alias>(&type)) { return a->identifier.str(); }
    if (const auto* p = std::get_if<Pointer>(&type)) { return p->identifier.str(); }
    throw std::logic_error("Invalid std::variant state");
}

std::string type::Function::to_string() const {
    return this->identifier.name;
}

std::string type::StructField::to_string() const {
    return this->identifier.name;
}

std::string type::Struct::to_string() const {
    return this->identifier.name;
}

std::string type::Alias::to_string() const {
    return this->identifier.name;
}

std::string type::Pointer::to_string() const {
    return this->identifier.name;
}

bool type::Type::same_as(const type::Type& other) const {
    return std::visit([this](auto&& t) { return this->same_as(t); }, other);
}

std::string type::Type::to_string() const {
    return std::visit(
        [](auto&& type) -> std::string { return type::to_string(type); }, *this
    );
}

bool type::Type::same_as(const BuiltinType t) const {
    if (const auto* builtin = std::get_if<BuiltinType>(this)) { return *builtin == t; }
    return false;
}

bool type::Type::same_as(const Function& f) const {
    if (const auto* func = std::get_if<Function>(this)) {
        /// TODO function equality resolution
        return true;
    }
    return false;
}

bool type::Type::same_as(const Struct& s) const {
    if (const auto* struc = std::get_if<Struct>(this)) {
        return struc->identifier == s.identifier;
    }
    return false;
}

bool type::Type::same_as(const Alias& a) const {
    if (const auto* alias = std::get_if<Alias>(this)) {
        return TypeRegistry::instance().get_type(alias->underlying).same_as(a);
    }
    return this->same_as(TypeRegistry::instance().get_type(a.underlying));
}

bool type::Type::same_as(const Pointer& p) const {
    if (const auto* ptr = std::get_if<Pointer>(this)) {
        return ptr->underlying == p.underlying;
    }
    return false;
}

type::TypeRegistry& type::TypeRegistry::instance() {
    static TypeRegistry instance;
    return instance;
}

void type::TypeRegistry::register_type(Type type) {
    if (std::ranges::find(types, type) != types.end()) {
        throw std::runtime_error("Type already registered");
    }
    types.emplace_back(std::move(type));
}

const type::Type& type::TypeRegistry::get_type(const TypeID id) const {
    if (id.id >= types.size()) { throw std::runtime_error("Type not found"); }
    return types[id.id];
}

type::TypeID type::TypeRegistry::get_type_id(const Type& type) const {
    const auto it = std::ranges::find(this->types, type);
    if (it == this->types.end()) { throw std::runtime_error("Type not found"); }

    return TypeID(std::distance(this->types.begin(), it));
}

type::TypeID type::TypeRegistry::get_type_id(std::string_view name) const {
    const auto it = std::ranges::find_if(this->types, [&name](const Type& t) -> bool {
        return t.to_string() == name;
    });

    if (it == this->types.end()) { throw std::runtime_error("Type not found"); }
    return TypeID(std::distance(this->types.begin(), it));
}

type::TypeID type::TypeRegistry::get_type_id(const ast::Type& type) const {
    if (const auto* builtin_type = std::get_if<BuiltinType>(&type);
        builtin_type != nullptr) {
        return this->get_type_id(Type{*builtin_type});
    }

    const auto& ident = std::get<AtomIdentifier>(type);
    return this->get_type_id(ident.name);
}

type::TypeRegistry::TypeRegistry() {
    /// Register builtin types
    this->register_type(Type{BuiltinType::I8});
    this->register_type(Type{BuiltinType::I16});
    this->register_type(Type{BuiltinType::I32});
    this->register_type(Type{BuiltinType::I64});

    this->register_type(Type{BuiltinType::BOOL});

    this->register_type(Type{BuiltinType::VOID});
    this->register_type(Type{BuiltinType::AUTO});
}

type::TypeID type::i8() noexcept {
    static TypeID i8_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::I8});
    return i8_id;
}

type::TypeID type::i16() noexcept {
    static TypeID i16_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::I16});
    return i16_id;
}

type::TypeID type::i32() noexcept {
    static TypeID i32_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::I32});
    return i32_id;
}

type::TypeID type::i64() noexcept {
    static TypeID i64_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::I64});
    return i64_id;
}

type::TypeID type::boolean() noexcept {
    static TypeID bool_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::BOOL});
    return bool_id;
}

type::TypeID type::unit() noexcept {
    static TypeID void_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::VOID});
    return void_id;
}

type::TypeID type::inferred() noexcept {
    static TypeID auto_id = TypeRegistry::instance().get_type_id(Type{BuiltinType::AUTO});
    return auto_id;
}

std::string to_string(const type::BuiltinType type) {
    // clang-format off
    static const std::map<type::BuiltinType, const char*> type_strings = {
        {type::BuiltinType::I64, "i64"},
        {type::BuiltinType::I32, "i32"},
        {type::BuiltinType::I16, "i16"},
        {type::BuiltinType::I8, "i8"},
        {type::BuiltinType::BOOL, "bool"},
        {type::BuiltinType::VOID, "void"},
        {type::BuiltinType::AUTO, "auto"},
    };
    // clang-format on
    if (!type_strings.contains(type)) { throw std::runtime_error("Unknown type"); }
    return type_strings.at(type);
}

std::string to_string(const type::TypeID id) {
    return std::string{id.get_name()};
}
