#include "native/native.hpp"

namespace nexus::native {

    bool resolveTarget(const std::string& target, NativeTarget& out) {
        if (target.empty() || target == "native") {
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__))
            out = NativeTarget::WindowsX64;
            return true;
#elif defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
            out = NativeTarget::LinuxX64;
            return true;
#else
            // The native target must be runnable by this host, not just emit-able.
            return false;
#endif
        }
        if (target == "x86_64-windows" || target == "x86_64-pc-windows-gnu" ||
            target == "x86_64-pc-windows-msvc") {
            out = NativeTarget::WindowsX64;
            return true;
        }
        if (target == "x86_64-linux" || target == "x86_64-unknown-linux-gnu") {
            out = NativeTarget::LinuxX64;
            return true;
        }
        return false;
    }

} // namespace nexus::native
