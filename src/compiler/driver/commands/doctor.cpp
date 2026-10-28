#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        int commandDoctor(bool json ) {
            // Mirror compileFile's compiler discovery order so portable builds
            // with a bundled wrapper are not reported as broken.
            std::vector<fs::path> compilerCandidates;
            if (const char* configured = std::getenv("NEXUS_CXX"); configured && *configured)
                compilerCandidates.emplace_back(configured);
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                for (const auto& rel : {fs::path("clang++.cmd"),
                                        fs::path("clang++.exe"),
                                        fs::path("clang++"),
                                        fs::path("../bin/clang++.cmd"),
                                        fs::path("../bin/clang++.exe"),
                                        fs::path("../bin/clang++"),
                                        fs::path("../toolchain/clang++.cmd")})
                    compilerCandidates.push_back(bin / rel);
            }
            compilerCandidates.emplace_back("clang++");

            std::string selectedCompiler;
            int compilerStatus = 127;
            for (const auto& candidate : compilerCandidates) {
                std::error_code ec;
                if (candidate.has_parent_path() && !fs::exists(candidate, ec))
                    continue;
                int rc = 127;
                try {
                    rc = runCommand({candidate.string(), "--version"}, fs::path(), true);
                } catch (const std::exception&) {
                    rc = 127;
                }
                if (rc == 0) {
                    selectedCompiler = candidate.string();
                    compilerStatus = 0;
                    break;
                }
            }

            const fs::path runtime = runtimeSource();
            const fs::path templateFile = templateSource("cli");
            fs::path stdlib;
            if (const char* configured = std::getenv("NEXUS_STD_DIR"); configured && *configured) {
                const fs::path candidate(configured);
                if (fs::exists(candidate / "io.nx"))
                    stdlib = candidate;
            }
            if (stdlib.empty() && !gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                for (const auto& candidate :
                     {bin / "../stdlib", bin / "stdlib", bin / "../share/nexus/stdlib"}) {
                    if (fs::is_regular_file(candidate / "io.nx")) {
                        stdlib = fs::weakly_canonical(candidate);
                        break;
                    }
                }
            }
#ifdef NEXUS_SOURCE_ROOT
            if (stdlib.empty() &&
                fs::is_regular_file(fs::path(NEXUS_SOURCE_ROOT) / "stdlib" / "io.nx"))
                stdlib = fs::path(NEXUS_SOURCE_ROOT) / "stdlib";
#endif

            const fs::path project = findProjectRoot(fs::current_path());
            const bool runtimeOk = !runtime.empty() && fs::exists(runtime);
            const bool templatesOk = !templateFile.empty() && fs::exists(templateFile);
            const bool stdlibOk = !stdlib.empty();
            const std::string compilerLabel =
                compilerStatus == 0 ? selectedCompiler : std::string();

#if defined(_WIN32) || defined(__linux__)
            const bool nativeHostSupported = hostIsX64();
#else
            const bool nativeHostSupported = false;
#endif
            if (json) {
                std::cout << "{\n"
                          << "  \"version\": \"" << kVersion << "\",\n"
                          << "  \"native_backend\": " << (nativeHostSupported ? "true" : "false")
                          << ",\n"
                          << "  \"llvm_backend\": " << (compilerStatus == 0 ? "true" : "false")
                          << ",\n"
                          << "  \"compiler_found\": " << (compilerStatus == 0 ? "true" : "false")
                          << ",\n"
                          << "  \"runtime\": " << (runtimeOk ? "true" : "false") << ",\n"
                          << "  \"stdlib\": " << (stdlibOk ? "true" : "false") << ",\n"
                          << "  \"templates\": " << (templatesOk ? "true" : "false") << ",\n"
                          << "  \"project\": " << (!project.empty() ? "true" : "false") << "\n"
                          << "}\n";
            } else {
                std::cout << "NEXUS doctor " << kVersion << "\n"
                          << "  executable:     "
                          << (gExecutable.empty() ? "unknown" : gExecutable.string()) << "\n"
                          << "  native backend: "
                          << (nativeHostSupported ? "OK (x86-64 built-in backend)"
                                                  : "unsupported on this host")
                          << "\n"
                          << "  LLVM backend:   "
                          << (compilerStatus == 0 ? "OK (" + compilerLabel + ")" : "MISSING")
                          << "\n"
                          << "  runtime:        " << (runtimeOk ? runtime.string() : "MISSING")
                          << "\n"
                          << "  stdlib:         " << (stdlibOk ? stdlib.string() : "MISSING")
                          << "\n"
                          << "  templates:      "
                          << (templatesOk ? templateFile.string() : "MISSING") << "\n"
                          << "  project:        "
                          << (project.empty() ? "not in a project" : project.string()) << "\n";
            }

            if (compilerStatus != 0 && !json)
                std::cout << "note: the built-in native backend handles only its supported subset; "
                             "other programs need clang++ or NEXUS_CXX.\n";
            if (!runtimeOk && !json)
                std::cerr << "warning: runtime.cpp/runtime.c was not found; LLVM fallback builds "
                             "may fail.\n";
            if (!templatesOk && !json)
                std::cerr
                    << "warning: project templates were not found; the new command may fail.\n";

            // LLVM is optional for programs supported by the built-in backend.
            return 0;
        }


        int commandExplainBackend(const fs::path& file, const std::string& target) {
            ModuleLoadContext loadContext;
            auto program = loadProgram(file, loadContext);
            SemanticAnalyzer().analyze(program);

            const std::string selectedTarget = target.empty() ? "native" : target;
            native::NativeTarget nativeTarget = native::NativeTarget::WindowsX64;
            bool targetSupported = native::resolveTarget(selectedTarget, nativeTarget);
            if (selectedTarget == "native" && !hostIsX64())
                targetSupported = false;

            std::string reason;
            const bool nativeSupported = targetSupported && native::supports(program, reason);

            std::cout << "NEXUS backend report\n"
                      << "  source: " << fs::absolute(file).string() << "\n"
                      << "  target: " << selectedTarget << "\n";
            if (nativeSupported) {
                std::cout << "  auto will select: builtin native\n"
                          << "  why: program fits the built-in x86-64 machine-code subset\n"
                          << "  toolchain: no external compiler required for this program\n";
            } else {
                if (!targetSupported) {
                    reason = "the built-in backend has no machine-code emitter for target '" +
                             selectedTarget + "'";
                }
                std::cout << "  auto will select: LLVM fallback\n"
                          << "  reason: " << reason << "\n"
                          << "  requirement: clang++ (or NEXUS_CXX / a bundled toolchain)\n"
                          << "  diagnose toolchain: nexus doctor\n";
            }
            return 0;
        }

    } // namespace driver
} // namespace nexus
