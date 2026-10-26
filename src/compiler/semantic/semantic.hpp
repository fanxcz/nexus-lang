#pragma once
#include "ast/ast.hpp"

#include <string>
#include <unordered_map>
#include <vector>
namespace nexus {

    struct Symbol {
        Type type{};
        bool mut = false;
    };

    class SemanticAnalyzer {

        std::unordered_map<std::string, Function*> funcs_;

        std::unordered_map<std::string, StructDecl*> structs_;

        std::unordered_map<std::string, EnumDecl*> enums_;

        std::vector<std::string> diagnostics_;

        long long discardCounter_ = 0;
        Type currentFunctionReturnType_ = Type::void_();

    public:
        void analyze(Program& p);

    private:
        Type expr(Expr* e, std::unordered_map<std::string, Symbol>& env);

        Type exprImpl(Expr* e, std::unordered_map<std::string, Symbol>& env);

        void block(Block& b, std::unordered_map<std::string, Symbol>& env, Type ret, bool inLoop);

        const StructDecl& getStruct(const std::string& n) const;

        const EnumDecl& getEnum(const std::string& n) const;

        // Parser produces Type::named for every identifier, so resolve names
        // against the declaration tables once they are known.
        void normalize(Type& type) const;
    };

} // namespace nexus
