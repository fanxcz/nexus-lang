#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
namespace nexus {

    struct Symbol {
        Type type{
        };
        bool mut=false;
    };

    class SemanticAnalyzer {

        std::unordered_map<std::string,Function*> funcs_;

        std::unordered_map<std::string,StructDecl*> structs_;

        std::unordered_map<std::string,EnumDecl*> enums_;

        public:
        void analyze(Program& p);

        private:
        Type expr(Expr* e,std::unordered_map<std::string,Symbol>& env);

        void block(Block& b,std::unordered_map<std::string,Symbol>& env,Type ret,bool inLoop);

        const StructDecl& getStruct(const std::string& n) const;

        const EnumDecl& getEnum(const std::string& n) const;

    };

}
