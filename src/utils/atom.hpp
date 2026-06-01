#pragma once

#include <memory>
#include <string>

struct Atom {
    virtual ~Atom() = default;

    constexpr auto operator<=>(const Atom&) const = default;
};

struct AtomIdentifier final : Atom {
    std::string name{};

    explicit AtomIdentifier(std::string name);
    explicit AtomIdentifier() = default;
    constexpr auto operator<=>(const AtomIdentifier&) const = default;
    [[nodiscard]] const std::string& str() const { return name; }
    [[nodiscard]] std::string str() { return name; }
    [[nodiscard]] std::unique_ptr<AtomIdentifier> clone() const;
};

struct AtomIntegerLiteral final : Atom {
    int64_t value;

    explicit AtomIntegerLiteral(int64_t value);
    constexpr auto operator<=>(const AtomIntegerLiteral&) const = default;
    [[nodiscard]] std::unique_ptr<AtomIntegerLiteral> clone() const;
};

struct AtomBooleanLiteral final : Atom {
    bool value;

    explicit AtomBooleanLiteral(bool value);
    constexpr auto operator<=>(const AtomBooleanLiteral&) const = default;
    [[nodiscard]] std::unique_ptr<AtomBooleanLiteral> clone() const;
};