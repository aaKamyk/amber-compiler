#include "operator.hpp"

#include <iostream>
#include <map>

std::ostream& operator<<(std::ostream& os, const lir::BinaryIntegerOperator& op) {
    static const std::map<lir::BinaryIntegerOperator, const char*> mapping = {
        {lir::BinaryIntegerOperator::ADD, "add"},
        {lir::BinaryIntegerOperator::SUB, "sub"},
        {lir::BinaryIntegerOperator::MUL, "mul"},
        {lir::BinaryIntegerOperator::UDIV, "udiv"},
        {lir::BinaryIntegerOperator::SDIV, "sdiv"},
        {lir::BinaryIntegerOperator::UREM, "urem"},
        {lir::BinaryIntegerOperator::SREM, "srem"},
    };
    if (mapping.contains(op)) { return os << mapping.at(op); }
    throw std::invalid_argument("Unknown binary integer operator");
}

std::ostream& operator<<(std::ostream& os, const lir::BinaryFloatingOperator& op) {
    static const std::map<lir::BinaryFloatingOperator, const char*> mapping = {
        {lir::BinaryFloatingOperator::FADD, "fadd"},
        {lir::BinaryFloatingOperator::FSUB, "fsub"},
        {lir::BinaryFloatingOperator::FMUL, "fmul"},
        {lir::BinaryFloatingOperator::FVID, "fdiv"},
        {lir::BinaryFloatingOperator::FREM, "frem"},
    };
    if (mapping.contains(op)) { return os << mapping.at(op); }
    throw std::invalid_argument("Unknown binary floating point operator");
}

std::ostream& operator<<(std::ostream& os, const lir::BinaryBitwiseOperator& op) {
    static const std::map<lir::BinaryBitwiseOperator, const char*> mapping = {
        {lir::BinaryBitwiseOperator::SHL, "shl"},
        {lir::BinaryBitwiseOperator::LSHR, "lshr"},
        {lir::BinaryBitwiseOperator::ASHR, "ashr"},
        {lir::BinaryBitwiseOperator::AND, "and"},
        {lir::BinaryBitwiseOperator::OR, "or"},
        {lir::BinaryBitwiseOperator::XOR, "xor"},
    };
    if (mapping.contains(op)) { return os << mapping.at(op); }
    throw std::invalid_argument("Unknown binary bitwise operator");
}
