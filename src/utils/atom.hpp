#pragma once

#include <memory>
#include <string>

struct Atom {
    virtual ~Atom() = default;

    auto operator<=>(const Atom&) const = default;
};

struct AtomIdentifier final : Atom {
    std::string name;

    explicit AtomIdentifier(std::string name);
    auto operator<=>(const AtomIdentifier&) const = default;
    [[nodiscard]] std::unique_ptr<AtomIdentifier> clone() const;
};

struct AtomIntegerLiteral final : Atom {
    int64_t value;

    explicit AtomIntegerLiteral(uint64_t value);
    [[nodiscard]] std::unique_ptr<AtomIntegerLiteral> clone() const;
};