#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        std::string targetTriple(const std::string& t) {

            if (t.empty() || t == "native")
                return {};

            if (t == "x86_64-linux")
                return "x86_64-unknown-linux-gnu";

            if (t == "aarch64-linux")
                return "aarch64-unknown-linux-gnu";

            if (t == "x86_64-windows")
                return "x86_64-pc-windows-gnu";

            if (t == "aarch64-windows")
                return "aarch64-pc-windows-msvc";

            if (t == "x86_64-macos")
                return "x86_64-apple-darwin";

            if (t == "aarch64-macos")
                return "arm64-apple-darwin";

            if (t == "aarch64-android")
                return "aarch64-linux-android";

            if (t == "wasm32-wasi")
                return "wasm32-wasi";

            return t;
        }


        int compileFile(const fs::path& in,
                        const fs::path& out,
                        const std::string& target,
                        bool emitIR,
                        int opt,
                        const std::string& backend ,
                        bool quiet ) {

            ModuleLoadContext loadContext;

            auto prog = loadProgram(in, loadContext);

            SemanticAnalyzer().analyze(prog);

            if (backend != "auto" && backend != "native" && backend != "llvm")
                throw std::runtime_error("unknown backend: " + backend +
                                         " (expected auto|native|llvm)");

            // Builtin direct machine-code backend: no LLVM, Clang, assembler,
            // or linker involved. Only used for IR-independent output; with
            // --emit-ir the LLVM IR text is requested explicitly.
            if (!emitIR && backend != "llvm") {
                native::NativeTarget nativeTarget = native::NativeTarget::WindowsX64;
                bool known = native::resolveTarget(target, nativeTarget);
                if ((target.empty() || target == "native") && !hostIsX64())
                    known = false;
                if (backend == "native" && !known)
                    throw std::runtime_error(
                        "builtin native backend has no emitter for target '" + target +
                        "' (x86_64 Windows/Linux are supported; use --backend llvm)");
                if (known) {
                    std::string reason;
                    if (native::supports(prog, reason)) {
                        const auto image = native::buildExecutable(prog, nativeTarget);
                        const fs::path parent = out.parent_path();
                        fs::create_directories(parent.empty() ? fs::current_path() : parent);
                        writeBinaryRetry(out, image.data(), image.size());
#ifndef _WIN32
                        std::error_code ec;
                        fs::permissions(out,
                                        fs::perms::owner_exec | fs::perms::group_exec |
                                            fs::perms::others_exec,
                                        fs::perm_options::add,
                                        ec);
#endif
                        if (!quiet) {
                            std::cout << "builtin native backend: " << image.size() << " bytes -> "
                                      << out << "\n";
                        }
                        return 0;
                    }
                    if (backend == "native")
                        throw std::runtime_error(reason);
                }
            }

            CodegenOptions cg{targetTriple(target), opt};

            const auto ir = LLVMIRGenerator(cg).generate(prog);

            fs::create_directories(out.parent_path().empty() ? fs::current_path()
                                                             : out.parent_path());

            fs::path irPath = out;

            irPath += ".ll";

            std::ofstream irf(irPath, std::ios::binary);

            if (!irf)
                throw std::runtime_error("cannot write " + irPath.string());

            irf << ir;

            irf.close();

            if (emitIR) {

                std::cout << ir;

                return 0;
            }
            // Select the native backend without requiring a global compiler install.
            // NEXUS_CXX explicitly overrides discovery; packaged builds place the
            // Zig-backed clang++ wrapper beside nexus-core.exe.
            std::string nativeCompiler;
            if (const char* configured = std::getenv("NEXUS_CXX"); configured && *configured) {
                nativeCompiler = configured;
            } else {
                std::vector<fs::path> candidates;
                if (!gExecutable.empty()) {
                    const fs::path executableDir = fs::weakly_canonical(gExecutable).parent_path();
                    candidates.push_back(executableDir / "clang++.cmd");
                    candidates.push_back(executableDir / "clang++.exe");
                    candidates.push_back(executableDir / "clang++");
                    candidates.push_back(executableDir / "../bin/clang++.cmd");
                    candidates.push_back(executableDir / "../bin/clang++.exe");
                    candidates.push_back(executableDir / "../bin/clang++");
                    candidates.push_back(executableDir / "../toolchain/clang++.cmd");
                }
                for (const auto& candidate : candidates) {
                    std::error_code ec;
                    if (fs::is_regular_file(candidate, ec)) {
                        nativeCompiler = fs::absolute(candidate, ec).string();
                        if (!ec)
                            break;
                        nativeCompiler = candidate.string();
                        break;
                    }
                }
                // On development machines and Linux/macOS package managers, PATH
                // remains a supported fallback. The official Windows portable
                // package works from its bundled backend before reaching this path.
                if (nativeCompiler.empty())
                    nativeCompiler = "clang++";
            }

            std::vector<std::string> backendArgv;
            backendArgv.push_back(nativeCompiler);
            backendArgv.push_back("-std=c++20");
            backendArgv.push_back("-Wno-override-module");
#ifdef _WIN32
            // Quiet MSVC CRT deprecation noise from supported C stdlib calls in runtime.cpp.
            backendArgv.push_back("-D_CRT_SECURE_NO_WARNINGS");
#endif

#ifdef __linux__
            backendArgv.push_back("-ldl");
#endif
#ifdef _WIN32
            // shell32.lib for CommandLineToArgvW used by nexus_os_args_*
            backendArgv.push_back("-lshell32");
#endif
            if (!target.empty() && target != "native")
                backendArgv.push_back("--target=" + targetTriple(target));

            backendArgv.push_back(irPath.string());
            backendArgv.push_back(runtimeSource().string());
            backendArgv.push_back("-O" + std::to_string(opt));
            backendArgv.push_back("-o");
            backendArgv.push_back(out.string());

            // runCommand() launches the backend directly (CreateProcessW /
            // fork+exec) without a shell, so paths with spaces work and no
            // command interpreter is required.
            const int rc = runCommand(backendArgv);
            if (rc != 0) {
                throw std::runtime_error(
                    "native backend failed using '" + nativeCompiler +
                    "'; check the bundled toolchain, NEXUS_CXX override, or target sysroot");
            }

            return 0;
        }

    } // namespace driver
} // namespace nexus
