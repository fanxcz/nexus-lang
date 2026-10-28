#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        fs::path defaultOutput(const fs::path& input,
                               const std::string& explicitOutput,
                               bool release,
                               bool windowsTarget) {

            fs::path output;
            if (!explicitOutput.empty()) {
                output = fs::path(explicitOutput);
            } else {
                const fs::path projectRoot = findProjectRoot(input.parent_path());
                if (!projectRoot.empty() && fs::exists(projectRoot / "nexus.toml")) {
                    const auto cfg = loadProject(projectRoot);
                    output = projectRoot / "build" / (release ? "release" : "debug") / cfg.artifact;
                } else {
                    output = fs::current_path() / input.stem();
                }
            }

            // Clang/LLD's Windows linker emits an .exe when no extension is given,
            // but the old run path then tried to launch the extension-less name.
            if (windowsTarget && output.extension().empty())
                output += ".exe";
            return output;
        }


        bool hostIsX64() {
#if defined(_M_X64) || defined(__x86_64__)
            return true;
#else
            return false;
#endif
        }

    } // namespace driver
} // namespace nexus
