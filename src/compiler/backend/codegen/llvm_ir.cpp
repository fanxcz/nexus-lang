#include "codegen/llvm_ir.hpp"

#include "codegen/llvm_cg.hpp"

namespace nexus {

    std::string LLVMIRGenerator::generate(const Program& p) {
        return CG(options_).generate(p);
    }

} // namespace nexus
