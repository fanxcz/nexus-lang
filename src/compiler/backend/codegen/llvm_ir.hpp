#pragma once
#include "ast/ast.hpp"
#include <string>
namespace nexus {

    struct CodegenOptions {
        std::string targetTriple;
        int optimization=2;
    };

    class LLVMIRGenerator {
        public: explicit LLVMIRGenerator(CodegenOptions o={
    }):options_(std::move(o)){
    } std::string generate(const Program& p);
    private: CodegenOptions options_;
};

}
