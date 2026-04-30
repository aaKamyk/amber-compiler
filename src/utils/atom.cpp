#include "atom.hpp"

std::unique_ptr<Atom> Atom::clone() const {
    return std::visit(
        [](auto&& child) { return std::make_unique<Atom>(child->clone()); }, this->atom
    );
}

std::unique_ptr<AtomIdentifier> AtomIdentifier::clone() const {
    return std::make_unique<AtomIdentifier>(*this);
}

std::unique_ptr<AtomIntegerLiteral> AtomIntegerLiteral::clone() const {
    return std::make_unique<AtomIntegerLiteral>(*this);
}