#include "native/native.hpp"

#include "native/image.hpp"
#include "native/native_generator.hpp"
#include "native/native_internal.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexus::native {

    bool supports(const Program& program, std::string& reason) {
        reason.clear();
        if (!program.structs.empty()) {
            reason = "struct '" + program.structs.front().name +
                     "' is outside the builtin native v0 subset (use --backend llvm)";
            return false;
        }
        if (!program.enums.empty()) {
            reason = "enum '" + program.enums.front().name +
                     "' is outside the builtin native v0 subset (use --backend llvm)";
            return false;
        }
        std::map<std::string, detail::FnSig> fns;
        for (const auto& f : program.functions) {
            detail::FnSig sig;
            for (const auto& p : f.params) {
                detail::Kind k = detail::Kind::Void;
                if (!detail::kindOfType(p.type, k) || k == detail::Kind::Void) {
                    reason = "parameter '" + p.name + "' of '" + f.name +
                             "' has a type outside the builtin native v0 subset (only i64 and bool "
                             "are supported)";
                    return false;
                }
                sig.params.push_back(k);
            }
            if (!detail::kindOfType(f.ret, sig.ret)) {
                reason = "result type of '" + f.name +
                         "' is outside the builtin native v0 subset (only i64, bool, and void are "
                         "supported)";
                return false;
            }
            if (f.external) {
                reason = "extern function '" + f.name +
                         "' is outside the builtin native v0 subset (use --backend llvm)";
                return false;
            }
            fns[f.name] = sig;
        }
        if (!fns.contains("main")) {
            reason = "no main function: the builtin native v0 backend needs `fn main()` as the "
                     "entry point";
            return false;
        }
        for (const auto& f : program.functions) {
            std::map<std::string, detail::Kind> env;
            for (std::size_t i = 0; i < f.params.size(); ++i)
                env[f.params[i].name] = fns[f.name].params[i];
            if (f.body)
                detail::checkBlock(f.body.get(), env, fns, false, reason);
            if (!reason.empty()) {
                reason = "in function '" + f.name + "': " + reason;
                return false;
            }
        }
        return true;
    }

    std::vector<std::uint8_t> buildExecutable(const Program& program, NativeTarget target) {
        std::string reason;
        if (!supports(program, reason))
            throw std::runtime_error(reason);
        CodeImage image;
        if (target == NativeTarget::WindowsX64) {
            image.imports = {
                "GetStdHandle", "WriteFile", "ExitProcess", "VirtualAlloc", "ReadFile"};
        }
        detail::Generator gen(image, target);
        gen.generate(program);
        if (target == NativeTarget::WindowsX64)
            return writeWindowsPE(image);
        return writeLinuxELF(image);
    }

} // namespace nexus::native
