#include "atom.hpp"

AtomIdentifier::AtomIdentifier(std::string name)
    : name(std::move(name)) {}

std::unique_ptr<AtomIdentifier> AtomIdentifier::clone() const {
    return std::make_unique<AtomIdentifier>(*this);
}

AtomIntegerLiteral::AtomIntegerLiteral(const int64_t value)
    : value(value) {}

std::unique_ptr<AtomIntegerLiteral> AtomIntegerLiteral::clone() const {
    return std::make_unique<AtomIntegerLiteral>(*this);
}
AtomBooleanLiteral::AtomBooleanLiteral(bool value)
    : value(value) {}