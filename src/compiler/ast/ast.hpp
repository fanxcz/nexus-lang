#pragma once
#include "lexer/token.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nexus {

    enum class TypeKind {
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F32, F64,
        Bool, String, Void, Pointer, Struct, Enum, Array, Slice, MutableSlice, Unknown
    };

    inline bool isIntegerType(TypeKind kind) {
        switch (kind) {
        case TypeKind::I8: case TypeKind::I16: case TypeKind::I32: case TypeKind::I64:
        case TypeKind::U8: case TypeKind::U16: case TypeKind::U32: case TypeKind::U64:
            return true;
        default:
            return false;
        }
    }

    inline bool isSignedIntegerType(TypeKind kind) {
        return kind == TypeKind::I8 || kind == TypeKind::I16 ||
               kind == TypeKind::I32 || kind == TypeKind::I64;
    }

    inline bool isUnsignedIntegerType(TypeKind kind) {
        return kind == TypeKind::U8 || kind == TypeKind::U16 ||
               kind == TypeKind::U32 || kind == TypeKind::U64;
    }

    inline bool isFloatType(TypeKind kind) {
        return kind == TypeKind::F32 || kind == TypeKind::F64;
    }

    inline bool isNumericType(TypeKind kind) {
        return isIntegerType(kind) || isFloatType(kind);
    }

    inline unsigned integerBitWidth(TypeKind kind) {
        switch (kind) {
        case TypeKind::I8: case TypeKind::U8: return 8;
        case TypeKind::I16: case TypeKind::U16: return 16;
        case TypeKind::I32: case TypeKind::U32: return 32;
        case TypeKind::I64: case TypeKind::U64: return 64;
        default: return 0;
        }
    }

    struct Type {

        TypeKind kind = TypeKind::Unknown;

        std::string name;

        std::shared_ptr<Type> pointee;

        std::shared_ptr<Type> element;

        std::size_t arraySize = 0;

        static Type i8() { return {TypeKind::I8, {}, {}, {}, 0}; }
        static Type i16() { return {TypeKind::I16, {}, {}, {}, 0}; }
        static Type i32() { return {TypeKind::I32, {}, {}, {}, 0}; }
        static Type i64() { return {TypeKind::I64, {}, {}, {}, 0}; }
        static Type u8() { return {TypeKind::U8, {}, {}, {}, 0}; }
        static Type u16() { return {TypeKind::U16, {}, {}, {}, 0}; }
        static Type u32() { return {TypeKind::U32, {}, {}, {}, 0}; }
        static Type u64() { return {TypeKind::U64, {}, {}, {}, 0}; }
        static Type f32() { return {TypeKind::F32, {}, {}, {}, 0}; }
        static Type f64() { return {TypeKind::F64, {}, {}, {}, 0}; }
        static Type boolean() {
            return {TypeKind::Bool, {}, {}, {}, 0};
        }
        static Type string() {
            return {TypeKind::String, {}, {}, {}, 0};
        }
        static Type void_() {
            return {TypeKind::Void, {}, {}, {}, 0};
        }
        static Type ptr(Type t) {
            Type x;
            x.kind = TypeKind::Pointer;
            x.pointee = std::make_shared<Type>(std::move(t));
            return x;
        }
        static Type named(std::string n) {
            Type x;
            x.kind = TypeKind::Struct;
            x.name = std::move(n);
            return x;
        }
        static Type enumerated(std::string n) {
            Type x;
            x.kind = TypeKind::Enum;
            x.name = std::move(n);
            return x;
        }
        static Type slice(Type t) {
            Type x;
            x.kind = TypeKind::Slice;
            x.element = std::make_shared<Type>(std::move(t));
            return x;
        }
        static Type mutableSlice(Type t) {
            Type x;
            x.kind = TypeKind::MutableSlice;
            x.element = std::make_shared<Type>(std::move(t));
            return x;
        }
        static Type array(Type t, std::size_t n) {
            Type x;
            x.kind = TypeKind::Array;
            x.element = std::make_shared<Type>(std::move(t));
            x.arraySize = n;
            return x;
        }
    };

    inline bool sameType(const Type& a, const Type& b) {

        if (a.kind != b.kind)
            return false;

        if (a.kind == TypeKind::Struct || a.kind == TypeKind::Enum)
            return a.name == b.name;

        if (a.kind == TypeKind::Pointer)
            return a.pointee && b.pointee && sameType(*a.pointee, *b.pointee);

        if (a.kind == TypeKind::Array)
            return a.arraySize == b.arraySize && a.element && b.element &&
                   sameType(*a.element, *b.element);

        if (a.kind == TypeKind::Slice || a.kind == TypeKind::MutableSlice)
            return a.element && b.element && sameType(*a.element, *b.element);

        return true;
    }

    struct SourceSpan {
        SourcePos start{};
        SourcePos end{}; // Exclusive end position.
        bool valid = false;
        // Canonical source path; empty for in-memory snippets such as the REPL.
        std::string sourcePath;
    };

    struct Expr {
        SourceSpan span{};
        virtual ~Expr() = default;
        using Ptr = std::unique_ptr<Expr>;
    };

    struct IntExpr : Expr {
        // Store the magnitude unsigned so u64 literals can use the full range.
        // Negative literals are represented by `negative` only for the signed
        // minimum-value case, which cannot be parsed as a positive i64 first.
        std::uint64_t value;
        bool negative = false;
        // True when this literal is the magnitude operand of a contextual unary '-'.
        // It may be one larger than the positive maximum of the signed target type.
        bool negativeMagnitude = false;
        Type literalType = Type::i64();
        explicit IntExpr(std::uint64_t v, bool isNegative = false)
            : value(v), negative(isNegative) {}
    };

    struct FloatExpr : Expr {
        double value;
        Type literalType = Type::f64();
        explicit FloatExpr(double v) : value(v) {}
    };

    struct BoolExpr : Expr {
        bool value;
        explicit BoolExpr(bool v) : value(v) {}
    };

    struct StringExpr : Expr {
        std::string value;
        explicit StringExpr(std::string v) : value(std::move(v)) {}
    };

    struct VarExpr : Expr {
        std::string name;
        explicit VarExpr(std::string n) : name(std::move(n)) {}
    };

    struct EnumVariantExpr : Expr {
        std::string enumName, variant;
        Expr::Ptr payload;
        EnumVariantExpr(std::string e, std::string v)
            : enumName(std::move(e)), variant(std::move(v)) {}
    };

    struct TryExpr : Expr {
        Expr::Ptr expr;
        TryExpr(Expr::Ptr e) : expr(std::move(e)) {}
    };

    struct ArrayExpr : Expr {
        std::vector<Expr::Ptr> elements;
    };

    struct IndexExpr : Expr {
        Expr::Ptr object, index;
        IndexExpr(Expr::Ptr o, Expr::Ptr i) : object(std::move(o)), index(std::move(i)) {}
    };

    // A bounded slice expression, e.g. `values[start..end]`. The upper
    // bound is exclusive, matching NEXUS's existing half-open for-ranges.
    // Lowered to a non-owning { pointer, length } pair by the LLVM backend.
    struct SliceExpr : Expr {
        Expr::Ptr object, start, end;
        SliceExpr(Expr::Ptr o, Expr::Ptr s, Expr::Ptr e)
            : object(std::move(o)), start(std::move(s)), end(std::move(e)) {}
    };

    struct UnaryExpr : Expr {
        TokenKind op;
        Expr::Ptr rhs;
        UnaryExpr(TokenKind o, Expr::Ptr r) : op(o), rhs(std::move(r)) {}
    };

    struct BinaryExpr : Expr {
        TokenKind op;
        Expr::Ptr lhs, rhs;
        BinaryExpr(TokenKind o, Expr::Ptr l, Expr::Ptr r)
            : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    };

    struct CallExpr : Expr {
        std::string callee;
        std::vector<Expr::Ptr> args;
        CallExpr(std::string c, std::vector<Expr::Ptr> a)
            : callee(std::move(c)), args(std::move(a)) {}
    };

    struct MemberExpr : Expr {
        Expr::Ptr object;
        std::string member;
        MemberExpr(Expr::Ptr o, std::string m) : object(std::move(o)), member(std::move(m)) {}
    };

    struct StructInitExpr : Expr {
        std::string typeName;
        std::vector<std::pair<std::string, Expr::Ptr>> fields;
        explicit StructInitExpr(std::string n) : typeName(std::move(n)) {}
    };

    struct Stmt {
        SourceSpan span{};
        virtual ~Stmt() = default;
        using Ptr = std::unique_ptr<Stmt>;
    };

    struct Block : Stmt {
        std::vector<Stmt::Ptr> statements;
    };

    struct LetStmt : Stmt {
        std::string name;
        bool mut = false;
        Expr::Ptr init;
        Type type{};
    };

    struct AssignStmt : Stmt {
        std::string name;
        Expr::Ptr value;
    };

    struct MemberAssignStmt : Stmt {
        Expr::Ptr object;
        std::string member;
        Expr::Ptr value;
        // TokenKind::Equal for a plain store; otherwise the arithmetic
        // operator of a compound assignment such as `+=`.
        TokenKind op = TokenKind::Equal;
    };

    struct IndexAssignStmt : Stmt {
        Expr::Ptr object, index, value;
        // TokenKind::Equal for a plain store; otherwise the arithmetic
        // operator of a compound assignment such as `+=`.
        TokenKind op = TokenKind::Equal;
    };

    // Assignment through a typed pointer, e.g. *output = value.
    struct DerefAssignStmt : Stmt {
        Expr::Ptr target, value;
        // TokenKind::Equal for a plain store; otherwise the arithmetic
        // operator of a compound assignment such as `+=`.
        TokenKind op = TokenKind::Equal;
    };

    struct ExprStmt : Stmt {
        Expr::Ptr expr;
    };

    // Evaluate call arguments now and invoke the void-returning call when the scope exits.
    struct DeferStmt : Stmt {
        Expr::Ptr call;
    };

    struct ReturnStmt : Stmt {
        Expr::Ptr expr;
    };

    struct IfStmt : Stmt {
        Expr::Ptr cond;
        std::unique_ptr<Block> thenBlock;
        std::unique_ptr<Block> elseBlock;
    };

    struct WhileStmt : Stmt {
        Expr::Ptr cond;
        std::unique_ptr<Block> body;
    };

    // `for name in start..end`, `for name in array`, or Unicode code points in a string.
    // Exactly one of (rangeEnd, collection) is set.
    struct ForStmt : Stmt {
        std::string name;
        Expr::Ptr start;
        Expr::Ptr rangeEnd;
        Expr::Ptr collection;
        std::unique_ptr<Block> body;
    };

    struct BreakStmt : Stmt {};
    struct ContinueStmt : Stmt {};

    struct MatchArm {
        Expr::Ptr pattern;
        // For range patterns (`start..end`), pattern holds the lower bound and
        // rangeEnd the inclusive upper bound.
        Expr::Ptr rangeEnd;
        // For binding patterns (`value =>`), the matched value is stored in a
        // local variable with this name for the guard and the arm body.
        std::string binding;
        // Optional `if condition` guard evaluated after the pattern matches.
        Expr::Ptr guard;
        std::unique_ptr<Block> body;
        bool wildcard = false;
    };

    struct MatchStmt : Stmt {
        Expr::Ptr value;
        std::vector<MatchArm> arms;
    };

    struct Param {
        std::string name;
        Type type;
        SourceSpan span{};
    };

    struct StructField {
        std::string name;
        Type type;
        SourceSpan span{};
    };

    struct StructDecl {
        std::string name;
        std::vector<StructField> fields;
        SourceSpan span{};
    };

    struct EnumVariant {
        std::string name;
        std::optional<Type> payload;
        SourceSpan span{};
    };

    struct EnumDecl {
        std::string name;
        std::vector<EnumVariant> variants;
        SourceSpan span{};
    };

    struct Function {
        std::string name;
        std::vector<Param> params;
        Type ret;
        std::unique_ptr<Block> body;
        bool external = false;
        SourceSpan span{};
    };

    struct Program {
        std::vector<std::string> imports;
        std::vector<StructDecl> structs;
        std::vector<EnumDecl> enums;
        std::vector<Function> functions;
    };

} // namespace nexus
