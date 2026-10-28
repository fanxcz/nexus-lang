#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        fs::path runtimeSource() {
            if (const char* env = std::getenv("NEXUS_RUNTIME_DIR")) {
                fs::path candidate = fs::path(env) / "runtime.cpp";
                if (fs::is_regular_file(candidate))
                    return fs::weakly_canonical(candidate);
                candidate = fs::path(env) / "runtime.c";
                if (fs::is_regular_file(candidate))
                    return fs::weakly_canonical(candidate);
            }
#ifdef NEXUS_SOURCE_ROOT
            const fs::path sourceRuntime = fs::path(NEXUS_SOURCE_ROOT) / "src/runtime/runtime.cpp";
            if (fs::is_regular_file(sourceRuntime))
                return fs::weakly_canonical(sourceRuntime);
#endif
#ifdef NEXUS_INSTALL_RUNTIME_DIR
            // Prefer the runtime that "cmake --install" refreshed over older
            // side-by-side copies that packaging scripts may have left behind.
            const fs::path installed = fs::path(NEXUS_INSTALL_RUNTIME_DIR) / "runtime.cpp";
            if (fs::is_regular_file(installed))
                return fs::weakly_canonical(installed);
#endif
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                const std::vector<fs::path> candidates = {bin / "../runtime/runtime.cpp",
                                                          bin /
                                                              "../share/nexus/runtime/runtime.cpp",
                                                          bin / "runtime/runtime.cpp",
                                                          bin / "../../runtime/runtime.cpp",
                                                          bin / "../runtime/runtime.c"};
                for (const auto& candidate : candidates) {
                    std::error_code ec;
                    if (fs::is_regular_file(candidate, ec))
                        return fs::weakly_canonical(candidate, ec);
                }
            }
            const fs::path local = fs::current_path() / "runtime/runtime.cpp";
            if (fs::is_regular_file(local))
                return fs::weakly_canonical(local);
            throw std::runtime_error(
                "Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
        }


        // Directory that holds the optional SDL runtime DLLs. Unlike runtime.cpp,
        // the DLLs are only present in packaged/installed layouts.
        fs::path runtimeLibraryDir() {
            std::vector<fs::path> candidates;
            if (const char* env = std::getenv("NEXUS_RUNTIME_DIR"))
                candidates.push_back(fs::path(env));
#ifdef NEXUS_INSTALL_RUNTIME_DIR
            // Installed layouts keep the SDL DLLs in <prefix>/runtime while the
            // compiler sources live in <prefix>/share/nexus/runtime.
            candidates.push_back(fs::path(NEXUS_INSTALL_RUNTIME_DIR)
                                     .parent_path()
                                     .parent_path()
                                     .parent_path() /
                                 "runtime");
#endif
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                candidates.push_back(bin / "../runtime");
                candidates.push_back(bin / "../share/nexus/runtime");
            }
            try {
                candidates.push_back(runtimeSource().parent_path());
            } catch (const std::exception&) {
                // The compiler already fails earlier when the runtime is missing.
            }
            for (const auto& candidate : candidates) {
                std::error_code ec;
                if (fs::is_regular_file(candidate / "SDL2.dll", ec))
                    return fs::weakly_canonical(candidate, ec);
            }
            return {};
        }


        // Copy the SDL DLLs next to a freshly built executable so double-clicking
        // it works without editing PATH. No-op when no SDL runtime is installed.
        void deployRuntimeLibraries(const fs::path& output) {
            const fs::path source = runtimeLibraryDir();
            if (source.empty())
                return;
            const fs::path destination = output.parent_path().empty() ? fs::path(".")
                                                                      : output.parent_path();
            for (const char* name :
                 {"SDL2.dll", "SDL2_image.dll", "SDL2_mixer.dll", "SDL2_ttf.dll"}) {
                std::error_code ec;
                const fs::path from = source / name;
                if (!fs::is_regular_file(from, ec))
                    continue;
                const fs::path to = destination / name;
                fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
            }
        }

    } // namespace driver
} // namespace nexus
