#pragma once

#include <memory>
#include <string>
#include <variant>

struct AtomIdentifier;
struct AtomIntegerLiteral;

struct Atom {
    using At = std::variant<std::unique_ptr<AtomIdentifier>, std::unique_ptr<AtomIntegerLiteral>>;
    At atom;

    [[nodiscard]] std::unique_ptr<Atom> clone() const;
};

struct AtomIdentifier {
    std::string name;

    [[nodiscard]] std::unique_ptr<AtomIdentifier> clone() const;
};

struct AtomIntegerLiteral {
    uint64_t value;

    [[nodiscard]] std::unique_ptr<AtomIntegerLiteral> clone() const;
};