#pragma once

#include "ast/ast.hpp"

#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace nexus {

    // Shared semantic helpers. They are inline so the analyzer can be split
    // across several translation units without a single definition file.

    inline std::string sourceLocation(const SourceSpan& span) {
        if (!span.valid)
            return {};
        std::string location = " at ";
        if (!span.sourcePath.empty())
            location += span.sourcePath + ":";
        location += std::to_string(span.start.line) + ":" +
                    std::to_string(span.start.column);
        return location;
    }

    inline bool containsVoidType(const Type& type) {
        if (type.kind == TypeKind::Void)
            return true;
        if (type.kind == TypeKind::Pointer)
            return type.pointee && containsVoidType(*type.pointee);
        if (type.kind == TypeKind::Array || type.kind == TypeKind::Slice ||
            type.kind == TypeKind::MutableSlice)
            return type.element && containsVoidType(*type.element);
        return false;
    }

    inline bool typeContainsSlice(const Type& type) {
        if (type.kind == TypeKind::Slice || type.kind == TypeKind::MutableSlice)
            return true;
        if (type.kind == TypeKind::Pointer)
            return type.pointee && typeContainsSlice(*type.pointee);
        if (type.kind == TypeKind::Array)
            return type.element && typeContainsSlice(*type.element);
        return false;
    }

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
        if (const auto* slice = dynamic_cast<const SliceExpr*>(expr))
            return lvalueRootVariable(slice->object.get());
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
        if (const auto* literal = dynamic_cast<const IntExpr*>(expr)) {
            constexpr auto signedMax = static_cast<std::uint64_t>(
                std::numeric_limits<long long>::max());
            if (literal->negative) {
                constexpr std::uint64_t minMagnitude = std::uint64_t{1} << 63;
                if (literal->value == minMagnitude)
                    return std::numeric_limits<long long>::min();
                if (literal->value > signedMax)
                    return std::nullopt;
                return -static_cast<long long>(literal->value);
            }
            if (literal->value > signedMax)
                return std::nullopt;
            return static_cast<long long>(literal->value);
        }
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

    inline bool isNumericLiteralExpression(const Expr* expression) {
        if (dynamic_cast<const IntExpr*>(expression) ||
            dynamic_cast<const FloatExpr*>(expression))
            return true;
        if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression))
            return unary->op == TokenKind::Minus &&
                   isNumericLiteralExpression(unary->rhs.get());
        if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
            switch (binary->op) {
            case TokenKind::Plus:
            case TokenKind::Minus:
            case TokenKind::Star:
            case TokenKind::Slash:
            case TokenKind::Percent:
                return isNumericLiteralExpression(binary->lhs.get()) &&
                       isNumericLiteralExpression(binary->rhs.get());
            default:
                return false;
            }
        }
        return false;
    }

    // Contextual typing is deliberately limited to numeric literal expressions.
    // Variables and computed values never silently change signedness or width.
    inline bool applyNumericLiteralType(Expr* expression, const Type& target) {
        if (!expression)
            return false;
        if (auto* literal = dynamic_cast<IntExpr*>(expression)) {
            if (!isIntegerType(target.kind))
                return false;
            const unsigned bits = integerBitWidth(target.kind);
            if (literal->negativeMagnitude) {
                if (!isSignedIntegerType(target.kind) || literal->negative ||
                    literal->value > (std::uint64_t{1} << (bits - 1)))
                    return false;
                literal->literalType = target;
                return true;
            }
            if (isSignedIntegerType(target.kind)) {
                const std::uint64_t limit = std::uint64_t{1} << (bits - 1);
                if ((!literal->negative && literal->value >= limit) ||
                    (literal->negative && literal->value > limit))
                    return false;
            } else {
                if (literal->negative)
                    return false;
                const std::uint64_t limit = bits == 64
                    ? std::numeric_limits<std::uint64_t>::max()
                    : ((std::uint64_t{1} << bits) - 1);
                if (literal->value > limit)
                    return false;
            }
            literal->literalType = target;
            return true;
        }
        if (auto* literal = dynamic_cast<FloatExpr*>(expression)) {
            if (!isFloatType(target.kind))
                return false;
            if (target.kind == TypeKind::F32 &&
                (!std::isfinite(literal->value) ||
                 std::abs(literal->value) > std::numeric_limits<float>::max()))
                return false;
            literal->literalType = target;
            return true;
        }
        if (auto* array = dynamic_cast<ArrayExpr*>(expression)) {
            if (target.kind != TypeKind::Array || !target.element ||
                array->elements.size() != target.arraySize)
                return false;
            for (auto& item : array->elements) {
                if (dynamic_cast<ArrayExpr*>(item.get()) ||
                    isNumericLiteralExpression(item.get())) {
                    if (!applyNumericLiteralType(item.get(), *target.element))
                        return false;
                }
            }
            return true;
        }
        if (auto* unary = dynamic_cast<UnaryExpr*>(expression)) {
            if (unary->op != TokenKind::Minus ||
                (!isSignedIntegerType(target.kind) && !isFloatType(target.kind)))
                return false;
            if (auto* integer = dynamic_cast<IntExpr*>(unary->rhs.get())) {
                if (!isSignedIntegerType(target.kind))
                    return false;
                const auto limit = std::uint64_t{1} << (integerBitWidth(target.kind) - 1);
                if (integer->value > limit)
                    return false;
                integer->literalType = target;
                integer->negativeMagnitude = true;
                return true;
            }
            if (auto* floating = dynamic_cast<FloatExpr*>(unary->rhs.get())) {
                if (!isFloatType(target.kind))
                    return false;
                if (target.kind == TypeKind::F32 &&
                    (!std::isfinite(floating->value) ||
                     std::abs(floating->value) > std::numeric_limits<float>::max()))
                    return false;
                floating->literalType = target;
                return true;
            }
            return applyNumericLiteralType(unary->rhs.get(), target);
        }
        if (auto* binary = dynamic_cast<BinaryExpr*>(expression)) {
            if (binary->op != TokenKind::Plus && binary->op != TokenKind::Minus &&
                binary->op != TokenKind::Star && binary->op != TokenKind::Slash &&
                binary->op != TokenKind::Percent)
                return false;
            // Only retype an expression when both sides are literal expressions;
            // this avoids turning `existing_i64 + 1` into an implicit conversion.
            return applyNumericLiteralType(binary->lhs.get(), target) &&
                   applyNumericLiteralType(binary->rhs.get(), target);
        }
        return false;
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
        case TypeKind::I8: return "i8";
        case TypeKind::I16: return "i16";
        case TypeKind::I32: return "i32";
        case TypeKind::I64: return "i64";
        case TypeKind::U8: return "u8";
        case TypeKind::U16: return "u16";
        case TypeKind::U32: return "u32";
        case TypeKind::U64: return "u64";
        case TypeKind::F32: return "f32";
        case TypeKind::F64: return "f64";
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
        case TypeKind::Slice:
            return type.element ? "[]" + describeType(*type.element) : "[]unknown";
        case TypeKind::MutableSlice:
            return type.element ? "mut []" + describeType(*type.element) : "mut []unknown";
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
            supported = isNumericType(target.kind) ||
                        target.kind == TypeKind::String;
            break;
        case TokenKind::Minus:
            display = "-=";
            supported = isNumericType(target.kind);
            break;
        case TokenKind::Star:
            display = "*=";
            supported = isNumericType(target.kind);
            break;
        case TokenKind::Slash:
            display = "/=";
            supported = isNumericType(target.kind);
            break;
        case TokenKind::Percent:
            display = "%=";
            supported = isNumericType(target.kind);
            break;
        case TokenKind::Ampersand:
            display = "&=";
            supported = isIntegerType(target.kind);
            break;
        case TokenKind::Pipe:
            display = "|=";
            supported = isIntegerType(target.kind);
            break;
        case TokenKind::Caret:
            display = "^=";
            supported = isIntegerType(target.kind);
            break;
        case TokenKind::ShiftLeft:
            display = "<<=";
            supported = isIntegerType(target.kind);
            break;
        case TokenKind::ShiftRight:
            display = ">>=";
            supported = isIntegerType(target.kind);
            break;
        default:
            break;
        }
        if (!supported)
            throw std::runtime_error(std::string("operator '") + display +
                                     "' cannot be applied to '" + describeType(target) + "'");
    }


} // namespace nexus
