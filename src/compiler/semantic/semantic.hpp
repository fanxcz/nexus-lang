#pragma once
#include "ast/ast.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace nexus {

    struct BorrowOrigin {
        std::size_t scopeDepth = 0;
        std::string binding;
    };

    struct Symbol {
        Type type{};
        bool mut = false;
        std::size_t scopeDepth = 0;
        // Tracks the shortest-lived lexical storage reachable through this value.
        // This is deliberately conservative for aggregates and is not a full borrow checker.
        std::optional<BorrowOrigin> borrowedFrom;
    };

    class SemanticAnalyzer {

        std::unordered_map<std::string, Function*> funcs_;

        std::unordered_map<std::string, StructDecl*> structs_;

        std::unordered_map<std::string, EnumDecl*> enums_;

        std::vector<std::string> diagnostics_;

        long long discardCounter_ = 0;
        Type currentFunctionReturnType_ = Type::void_();
        std::size_t scopeDepth_ = 0;

    public:
        void analyze(Program& p);

    private:
        Type expr(Expr* e, std::unordered_map<std::string, Symbol>& env);

        Type exprImpl(Expr* e, std::unordered_map<std::string, Symbol>& env);

        std::optional<BorrowOrigin> pointerOrigin(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env) const;

        std::optional<Type> provenanceType(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env) const;

        bool typeMayContainPointer(const Type& type) const;

        void mergeBorrowState(std::unordered_map<std::string, Symbol>& destination,
                              const std::unordered_map<std::string, Symbol>& branch) const;

        void block(Block& b, std::unordered_map<std::string, Symbol>& env, Type ret, bool inLoop);

        const StructDecl& getStruct(const std::string& n) const;

        const EnumDecl& getEnum(const std::string& n) const;

        bool blockAlwaysReturns(const Block& block) const;

        // Parser produces Type::named for every identifier, so resolve names
        // against the declaration tables once they are known.
        void normalize(Type& type) const;
    };

} // namespace nexus
