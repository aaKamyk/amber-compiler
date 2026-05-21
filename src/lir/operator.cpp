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

std::ostream& operator<<(std::ostream& os, const lir::BinaryFloatingPointOperator& op) {
    static const std::map<lir::BinaryFloatingPointOperator, const char*> mapping = {
        {lir::BinaryFloatingPointOperator::FADD, "fadd"},
        {lir::BinaryFloatingPointOperator::FSUB, "fsub"},
        {lir::BinaryFloatingPointOperator::FMUL, "fmul"},
        {lir::BinaryFloatingPointOperator::FVID, "fdiv"},
        {lir::BinaryFloatingPointOperator::FREM, "frem"},
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

std::ostream& operator<<(std::ostream& os, const lir::IntegerComparisonOperator& op) {
    static const std::map<lir::IntegerComparisonOperator, const char*> mapping = {
        {lir::IntegerComparisonOperator::EQ, "eq"},
        {lir::IntegerComparisonOperator::NE, "ne"},
        {lir::IntegerComparisonOperator::UGT, "ugt"},
        {lir::IntegerComparisonOperator::UGE, "uge"},
        {lir::IntegerComparisonOperator::ULT, "ult"},
        {lir::IntegerComparisonOperator::ULE, "ule"},
        {lir::IntegerComparisonOperator::SGT, "sgt"},
        {lir::IntegerComparisonOperator::SGE, "sge"},
        {lir::IntegerComparisonOperator::SLT, "slt"},
        {lir::IntegerComparisonOperator::SLE, "sle"},
    };
    if (mapping.contains(op)) { return os << mapping.at(op); }
    throw std::invalid_argument("Unknown integer comparison operator");
}

std::ostream& operator<<(std::ostream& os, const lir::FloatingPointComparisonOperator& op) {
    static const std::map<lir::FloatingPointComparisonOperator, const char*> mapping = {
        {lir::FloatingPointComparisonOperator::FALSE, "false"},
        {lir::FloatingPointComparisonOperator::OEQ, "oeq"},
        {lir::FloatingPointComparisonOperator::OGT, "ogt"},
        {lir::FloatingPointComparisonOperator::OGE, "oge"},
        {lir::FloatingPointComparisonOperator::OLT, "olt"},
        {lir::FloatingPointComparisonOperator::OLE, "ole"},
        {lir::FloatingPointComparisonOperator::ONE, "one"},
        {lir::FloatingPointComparisonOperator::ORD, "ord"},
        {lir::FloatingPointComparisonOperator::UEQ, "ueq"},
        {lir::FloatingPointComparisonOperator::UGT, "ugt"},
        {lir::FloatingPointComparisonOperator::UGE, "uge"},
        {lir::FloatingPointComparisonOperator::ULT, "ult"},
        {lir::FloatingPointComparisonOperator::ULE, "ule"},
        {lir::FloatingPointComparisonOperator::UNE, "une"},
        {lir::FloatingPointComparisonOperator::UNO, "uno"},
        {lir::FloatingPointComparisonOperator::TRUE, "true"},
    };
    if (mapping.contains(op)) { return os << mapping.at(op); }
    throw std::invalid_argument("Unknown integer comparison operator");
}
