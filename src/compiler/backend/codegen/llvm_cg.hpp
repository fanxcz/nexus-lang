#pragma once

#include "codegen/llvm_ir.hpp"

#include "builtins.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace nexus {

    // LLVM IR emitter for a validated program. The declaration lives here;
    // the implementation is split by topic under llvm_cg_*.cpp.
    class CG {

    public:
        explicit CG(CodegenOptions o);
        CodegenOptions options;
        std::ostringstream module, functionsIR, body;
        int reg = 0, label = 0;
        bool terminated = false;

        std::unordered_map<std::string, std::string> vars;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, Function*> functions;
        std::unordered_map<std::string, StructDecl*> structs;
        std::unordered_map<std::string, EnumDecl*> enums;

        std::vector<std::string> globals;
        Type currentReturnType = Type::void_();
        // Label of the basic block currently being emitted. Logical operators
        // need it to emit correct phi predecessors for short-circuiting.
        std::string currentBlock;
        std::vector<std::string> breakLabels, continueLabels;
        // Each entry represents one lexical scope; calls are stored as fully typed IR
        // instructions after their arguments have been evaluated at the defer site.
        std::vector<std::vector<std::string>> deferredCalls;
        std::vector<std::size_t> loopDeferScopeDepth;

        std::string next();
        std::string lab(const std::string& p);
        void emit(const std::string& s);
        void startBlock(const std::string& name);
        void emitModule(const std::string& s);
        static std::string llvmType(const Type& t);
        static std::string escape(const std::string& s);
        const StructDecl& getStruct(const std::string& n) const;
        const EnumDecl& getEnum(const std::string& n) const;
        std::size_t fieldIndex(const StructDecl& s, const std::string& n) const;
        long long enumTag(const EnumVariantExpr& e) const;
        Type typeOf(const Expr* e);
        std::string stringValue(const std::string& s);
        std::string loadVar(const std::string& name);
        std::string addressOf(const Expr* e);
        std::string memberLoad(const MemberExpr* m);
        std::string expr(const Expr* e);
        std::string deferredCall(const CallExpr* c);

        // Emit pending calls in reverse registration order. A depth of zero means
        // all active scopes; a loop records its entry depth so break/continue only
        // clean up scopes they actually leave.
        void emitDefersFrom(std::size_t keepDepth);

        // Emit `lhs <op> rhs` when both operands have already been evaluated.
        // Used by compound assignment, where the left operand is a loaded value
        // rather than an expression tree.
        std::string compoundValue(TokenKind op, const Type& type, const std::string& lhs,
                                  const std::string& rhs);

        bool stmtBlock(const Block* nested, Type ret);

        bool stmt(const Stmt* s, Type ret);
        std::string generate(const Program& p);
    };

} // namespace nexus
