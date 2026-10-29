#pragma once

#include "ast/ast.hpp"

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace nexus {

    // Shared semantic helpers. They are inline so the analyzer can be split
    // across several translation units without a single definition file.

    // Returns the root variable that owns an assignable storage location.
    // Taking an address yields a mutable pointer in the current type system, so
    // the root binding must itself be mutable. The type system does not yet have
    // const-qualified pointer types; allowing & of immutable storage would let
    // callers mutate it through a pointer and bypass the normal let check.
    inline const VarExpr* lvalueRootVariable(const Expr* expr) {
        if (const auto* variable = dynamic_cast<const VarExpr*>(expr))
            return variable;
        if (const auto* member = dynamic_cast<const MemberExpr*>(expr))
            return lvalueRootVariable(member->object.get());
        if (const auto* index = dynamic_cast<const IndexExpr*>(expr))
            return lvalueRootVariable(index->object.get());
        return nullptr;
    }

    inline const StructField* fieldOf(const StructDecl& s, const std::string& n) {
        for (auto& f : s.fields)
            if (f.name == n)
                return &f;
        return nullptr;
    }

    // Constant negative indices are parsed as UnaryExpr nodes, not IntExpr.
    // Evaluate only literal and unary-minus forms to reject obvious bad access
    // at compile time without guessing about more complex expressions.
    inline std::optional<long long> constantIntegerValue(const Expr* expr) {
        if (const auto* literal = dynamic_cast<const IntExpr*>(expr))
            return literal->value;
        if (const auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            if (unary->op != TokenKind::Minus)
                return std::nullopt;
            const auto value = constantIntegerValue(unary->rhs.get());
            if (!value || *value == std::numeric_limits<long long>::min())
                return std::nullopt;
            return -*value;
        }
        return std::nullopt;
    }

    inline void validateCompoundAssignDivisor(TokenKind op, const Expr* rhs) {
        if (op != TokenKind::Slash && op != TokenKind::Percent)
            return;
        const auto divisor = constantIntegerValue(rhs);
        if (divisor && *divisor == 0)
            throw std::runtime_error("division by zero");
    }

    inline bool constantIndexOutOfBounds(const Expr* index, size_t arraySize) {
        const auto value = constantIntegerValue(index);
        if (!value)
            return false;
        return *value < 0 || static_cast<size_t>(*value) >= arraySize;
    }

    inline std::string describeType(const Type& type) {
        switch (type.kind) {
        case TypeKind::I64:
            return "i64";
        case TypeKind::F64:
            return "f64";
        case TypeKind::Bool:
            return "bool";
        case TypeKind::String:
            return "string";
        case TypeKind::Void:
            return "void";
        case TypeKind::Unknown:
            return "unknown";
        case TypeKind::Struct:
            return type.name;
        case TypeKind::Enum:
            return "enum " + type.name;
        case TypeKind::Pointer:
            return type.pointee ? "*" + describeType(*type.pointee) : "*unknown";
        case TypeKind::Array:
            return type.element ? "[" + describeType(*type.element) + "; " +
                                      std::to_string(type.arraySize) + "]"
                                : "[unknown]";
        }
        return "unknown";
    }

    inline void validateCompoundAssign(TokenKind op, const Type& target) {
        if (op == TokenKind::Equal)
            return;
        bool supported = false;
        const char* display = "=";
        switch (op) {
        case TokenKind::Plus:
            display = "+=";
            supported = target.kind == TypeKind::I64 || target.kind == TypeKind::F64 ||
                        target.kind == TypeKind::String;
            break;
        case TokenKind::Minus:
            display = "-=";
            supported = target.kind == TypeKind::I64 || target.kind == TypeKind::F64;
            break;
        case TokenKind::Star:
            display = "*=";
            supported = target.kind == TypeKind::I64 || target.kind == TypeKind::F64;
            break;
        case TokenKind::Slash:
            display = "/=";
            supported = target.kind == TypeKind::I64 || target.kind == TypeKind::F64;
            break;
        case TokenKind::Percent:
            display = "%=";
            supported = target.kind == TypeKind::I64 || target.kind == TypeKind::F64;
            break;
        case TokenKind::Ampersand:
            display = "&=";
            supported = target.kind == TypeKind::I64;
            break;
        case TokenKind::Pipe:
            display = "|=";
            supported = target.kind == TypeKind::I64;
            break;
        case TokenKind::Caret:
            display = "^=";
            supported = target.kind == TypeKind::I64;
            break;
        case TokenKind::ShiftLeft:
            display = "<<=";
            supported = target.kind == TypeKind::I64;
            break;
        case TokenKind::ShiftRight:
            display = ">>=";
            supported = target.kind == TypeKind::I64;
            break;
        default:
            break;
        }
        if (!supported)
            throw std::runtime_error(std::string("operator '") + display +
                                     "' cannot be applied to '" + describeType(target) + "'");
    }


} // namespace nexus
