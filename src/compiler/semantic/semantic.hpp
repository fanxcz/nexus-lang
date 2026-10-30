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

    struct ManagedAllocationState {
        const Expr* allocation = nullptr;
        std::optional<std::size_t> parameter;
        bool freed = false;
    };

    struct Symbol {
        Type type{};
        bool mut = false;
        std::size_t scopeDepth = 0;
        // Tracks the shortest-lived lexical storage reachable through this value.
        // This is deliberately conservative for aggregates and is not a full borrow checker.
        std::optional<BorrowOrigin> borrowedFrom;
        // The syntactic mem_alloc site shared by direct local aliases, when known.
        const Expr* managedAllocation = nullptr;
        std::optional<std::size_t> managedAllocationParameter;
        bool managedAllocationFreed = false;
    };

    class SemanticAnalyzer {

        std::unordered_map<std::string, Function*> funcs_;

        std::unordered_map<std::string, StructDecl*> structs_;

        std::unordered_map<std::string, EnumDecl*> enums_;

        // Parameter indices whose pointer-bearing values may flow into a function result.
        std::unordered_map<std::string, std::vector<std::size_t>> returnedBorrowParams_;
        std::unordered_map<std::string, std::size_t> currentFunctionParameterIndices_;
        std::vector<std::size_t> currentFunctionReturnBorrowParams_;

        // Managed-handle summaries are intentionally limited to i64 return values
        // and direct parameter effects; opaque external functions remain unknown.
        std::unordered_map<std::string, std::vector<std::size_t>> returnedManagedParams_;
        std::unordered_map<std::string, bool> returnedManagedFresh_;
        std::unordered_map<std::string, bool> returnedManagedFreshFreed_;
        std::unordered_map<std::string, bool> returnedManagedUnknown_;
        std::unordered_map<std::string, std::vector<std::size_t>> freedManagedParams_;
        std::unordered_map<std::string, std::vector<std::size_t>> usedManagedParams_;
        std::vector<std::size_t> currentFunctionReturnManagedParams_;
        std::vector<std::size_t> currentFunctionFreedManagedParams_;
        std::vector<std::size_t> currentFunctionUsedManagedParams_;
        bool currentFunctionReturnManagedFresh_ = false;
        bool currentFunctionReturnManagedFreshFreed_ = false;
        bool currentFunctionReturnManagedUnknown_ = false;

        std::vector<std::string> diagnostics_;

        long long discardCounter_ = 0;
        Type currentFunctionReturnType_ = Type::void_();
        std::size_t scopeDepth_ = 0;
        bool analyzingDeferredCall_ = false;

    public:
        void analyze(Program& p);

    private:
        Type expr(Expr* e, std::unordered_map<std::string, Symbol>& env);

        Type exprImpl(Expr* e, std::unordered_map<std::string, Symbol>& env);

        std::optional<BorrowOrigin> pointerOrigin(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env) const;

        std::optional<ManagedAllocationState> managedAllocationOrigin(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env,
            bool trackAllocationSite) const;

        void checkManagedAllocationUse(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env,
            const std::string& operation) const;

        void recordManagedAllocationUse(
            const Expr* e, const std::unordered_map<std::string, Symbol>& env);

        void releaseManagedAllocation(const Expr* e,
                                      std::unordered_map<std::string, Symbol>& env,
                                      bool deferred);

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
