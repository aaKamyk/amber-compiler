#pragma once

#include <memory>
#include <string>
#include <variant>

struct AtomIdentifier;
struct AtomIntegerLiteral;

struct Atom {
    using At = std::variant<std::unique_ptr<AtomIdentifier>, std::unique_ptr<AtomIntegerLiteral>>;
    At atom;
};

struct AtomIdentifier {
    std::string name;
};

struct AtomIntegerLiteral {
    uint64_t value;
};