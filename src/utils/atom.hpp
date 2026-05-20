#pragma once

#include <memory>
#include <string>

struct Atom {
    virtual ~Atom() = default;
};

struct AtomIdentifier final : Atom {
    std::string name;

    explicit AtomIdentifier(std::string name);
    [[nodiscard]] std::unique_ptr<AtomIdentifier> clone() const;
};

struct AtomIntegerLiteral final : Atom {
    int64_t value;

    explicit AtomIntegerLiteral(uint64_t value);
    [[nodiscard]] std::unique_ptr<AtomIntegerLiteral> clone() const;
};