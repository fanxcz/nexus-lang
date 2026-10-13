#include "driver/driver.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegen/llvm_ir.hpp"
#include "native/native.hpp"
#include "format/formatter.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <chrono>
#include <cstdint>
#include <thread>
#include <algorithm>
#include <cctype>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#endif

#ifndef NEXUS_VERSION
#define NEXUS_VERSION "dev"
#endif

namespace nexus {

    namespace fs = std::filesystem;

    namespace {

        constexpr const char* kVersion = NEXUS_VERSION;

        fs::path gExecutable;

        fs::path gDiagnosticSource;
        std::string gDiagnosticText;

        std::string readFile(const fs::path& p) {

            std::ifstream f(p, std::ios::binary);

            if (!f) throw std::runtime_error("cannot open " + p.string());

            return {
                std::istreambuf_iterator<char>(f), {
                }
            };

        }

        std::string trim(std::string s) {

            const auto first = s.find_first_not_of(" \t\r\n");

            if (first == std::string::npos) return {
            };

            const auto last = s.find_last_not_of(" \t\r\n");

            return s.substr(first, last - first + 1);

        }

        std::string formatDiagnostic(const std::exception& error) {
            const std::string raw = error.what();
            const auto marker = raw.rfind(" at ");
            if (marker != std::string::npos) {
                const std::string_view location(raw.data() + marker + 4, raw.size() - marker - 4);
                const auto separator = location.find(':');
                if (separator != std::string_view::npos && location.find(':', separator + 1) == std::string_view::npos) {
                    const auto parseNumber = [](std::string_view value, std::size_t& result) {
                        if (value.empty()) return false;
                        result = 0;
                        for (const char c : value) {
                            if (c < '0' || c > '9') return false;
                            result = result * 10 + static_cast<std::size_t>(c - '0');
                        }
                        return result > 0;
                    };
                    std::size_t line = 0;
                    std::size_t column = 0;
                    if (parseNumber(location.substr(0, separator), line) &&
                        parseNumber(location.substr(separator + 1), column) &&
                        !gDiagnosticSource.empty()) {
                        std::ostringstream out;
                        out << gDiagnosticSource.string() << ":" << line << ":" << column
                            << ": error[NX1001]: " << trim(raw.substr(0, marker));
                        std::istringstream source(gDiagnosticText);
                        std::string sourceLine;
                        for (std::size_t current = 1; current <= line && std::getline(source, sourceLine); ++current) {
                            if (current == line) {
                                const std::string label = "  " + std::to_string(line) + " | ";
                                out << "\n" << label << sourceLine
                                    << "\n" << std::string(label.size(), ' ')
                                    << std::string(column - 1, ' ') << "^";
                                break;
                            }
                        }
                        return out.str();
                    }
                }
            }
            return "error[NX0000]: " + raw;
        }

        std::string tomlString(const std::string& data, const std::string& section, const std::string& key) {

            std::istringstream in(data);

            std::string line;

            std::string current;

            while (std::getline(in, line)) {

                line = trim(line);

                if (line.empty() || line[0] == '#') continue;

                if (line.front() == '[' && line.back() == ']') {

                    current = trim(line.substr(1, line.size() - 2));

                    continue;

                }
                if (current != section) continue;

                const auto eq = line.find('=');

                if (eq == std::string::npos) continue;

                const auto lhs = trim(line.substr(0, eq));

                if (lhs != key) continue;

                const auto rhs = trim(line.substr(eq + 1));

                if (rhs.size() >= 2 && rhs.front() == '"' && rhs.back() == '"') return rhs.substr(1, rhs.size() - 2);

                return rhs;

            }
            return {
            };

        }

        int64_t tomlInt(const std::string& data, const std::string& section, const std::string& key, int64_t fallback) {

            const std::string value = tomlString(data, section, key);

            if (value.empty()) return fallback;

            try {
                return std::stoll(value);
            } catch (...) {
                return fallback;
            }
        }

        struct ProjectConfig {

            fs::path root;

            std::string name;

            std::string version;

            std::string entry;

            std::string artifact;

            std::string target;

            int64_t windowWidth = 1280;

            int64_t windowHeight = 720;

            std::string windowTitle;

        };

        fs::path findProjectRoot(fs::path start) {

            std::error_code ec;

            start = fs::weakly_canonical(start, ec);

            if (ec) start = fs::absolute(start, ec);

            if (fs::is_regular_file(start, ec)) start = start.parent_path();

            for (;;) {

                if (fs::exists(start / "nexus.toml")) return start;

                const auto parent = start.parent_path();

                if (parent == start) break;

                start = parent;

            }
            return {
            };

        }

        ProjectConfig loadProject(const fs::path& requestedRoot = fs::current_path()) {

            const fs::path root = findProjectRoot(requestedRoot);

            if (root.empty()) throw std::runtime_error("nexus.toml not found; run `nexus init` or pass a .nx file");

            const std::string data = readFile(root / "nexus.toml");

            ProjectConfig cfg;

            cfg.root = root;

            cfg.name = tomlString(data, "package", "name");

            cfg.version = tomlString(data, "package", "version");

            cfg.entry = tomlString(data, "build", "entry");

            cfg.artifact = tomlString(data, "build", "artifact");

            cfg.target = tomlString(data, "build", "target");

            cfg.windowWidth = tomlInt(data, "window", "width", 1280);

            cfg.windowHeight = tomlInt(data, "window", "height", 720);

            cfg.windowTitle = tomlString(data, "window", "title");

            if (cfg.name.empty()) cfg.name = root.filename().string();

            if (cfg.version.empty()) cfg.version = kVersion;

            if (cfg.entry.empty()) cfg.entry = "src/main.nx";

            if (cfg.artifact.empty()) cfg.artifact = cfg.name;

            if (cfg.windowTitle.empty()) cfg.windowTitle = cfg.name;

            return cfg;

        }

        // Launch child processes directly instead of routing through a shell.
        // The previous implementation built a single command string and passed
        // it to std::system(), which breaks for quoted program paths containing
        // spaces (cmd.exe strips the outer quotes) and needlessly depends on a
        // command interpreter. runCommand() uses CreateProcessW on Windows and
        // fork()/execvp() on POSIX, so no shell quoting is required at all.
        // Returns the child exit code, or 127 when the program cannot start.
        int runCommand(const std::vector<std::string>& argv, const fs::path& workDir = fs::path(), bool quiet = false) {
            if (argv.empty()) throw std::runtime_error("cannot run an empty command");
#ifdef _WIN32
            const auto widen = [](const std::string& s) {
                if (s.empty()) return std::wstring();
                const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
                std::wstring w(static_cast<std::size_t>(n), L'\0');
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), w.data(), n);
                return w;
            };
            // Quote one element following CommandLineToArgvW rules.
            const auto quoteArg = [&](const std::string& arg) {
                bool need = arg.empty();
                for (const char c : arg) {
                    if (c == ' ' || c == '\t' || c == '"' || c == '\n') {
                        need = true;
                        break;
                    }
                }
                std::wstring w = widen(arg);
                if (!need) return w;
                std::wstring out;
                out.push_back(L'"');
                std::size_t backslashes = 0;
                for (const wchar_t c : w) {
                    if (c == L'\\') {
                        ++backslashes;
                        out.push_back(c);
                    } else if (c == L'"') {
                        for (std::size_t i = 0; i < backslashes; ++i) out.push_back(L'\\');
                        backslashes = 0;
                        out.push_back(L'\\');
                        out.push_back(L'"');
                    } else {
                        backslashes = 0;
                        out.push_back(c);
                    }
                }
                for (std::size_t i = 0; i < backslashes; ++i) out.push_back(L'\\');
                out.push_back(L'"');
                return out;
            };

            std::vector<std::string> full = argv;
            std::wstring launcher;
            const std::string ext = fs::path(argv[0]).extension().string();
            const bool isBatch = ext == ".cmd" || ext == ".CMD" ||
                ext == ".bat" || ext == ".BAT";
            if (isBatch) {
                // Batch wrappers (for example the Zig-backed clang++.cmd) cannot
                // be started with CreateProcess directly; route them through
                // `cmd.exe /d /c call "..."` with every element quoted exactly
                // once. This is the only place where cmd.exe is still involved,
                // and only for .cmd/.bat wrappers.
                full.clear();
                full.push_back("cmd.exe");
                full.push_back("/d");
                full.push_back("/c");
                full.push_back("call");
                for (const auto& a : argv) full.push_back(a);
            }
            std::wstring commandLine;
            for (std::size_t i = 0; i < full.size(); ++i) {
                if (i) commandLine.push_back(L' ');
                commandLine += quoteArg(full[i]);
            }
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            HANDLE nul = INVALID_HANDLE_VALUE;
            if (quiet) {
                nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (nul != INVALID_HANDLE_VALUE) {
                    startup.hStdOutput = nul;
                    startup.hStdError = nul;
                    startup.dwFlags |= STARTF_USESTDHANDLES;
                }
            }
            PROCESS_INFORMATION process{};
            const std::wstring workDirW = workDir.empty() ? std::wstring() : widen(fs::absolute(workDir).string());
            const BOOL started = CreateProcessW(
                nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                workDir.empty() ? nullptr : workDirW.c_str(), &startup, &process);
            const DWORD spawnError = GetLastError();
            if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
            if (!started) {
                std::cerr << "nexus: cannot start '" << argv[0]
                    << "' (Windows error " << spawnError << ")\n";
                return 127;
            }
            CloseHandle(process.hThread);
            WaitForSingleObject(process.hProcess, INFINITE);
            DWORD exitCode = 1;
            GetExitCodeProcess(process.hProcess, &exitCode);
            CloseHandle(process.hProcess);
            return static_cast<int>(exitCode);
#else
            pid_t pid = fork();
            if (pid < 0) {
                std::cerr << "nexus: cannot fork for '" << argv[0] << "': "
                    << std::strerror(errno) << "\n";
                return 127;
            }
            if (pid == 0) {
                if (!workDir.empty() && chdir(workDir.c_str()) != 0) _exit(127);
                if (quiet) {
                    const int fd = open("/dev/null", O_WRONLY);
                    if (fd >= 0) {
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        if (fd > STDERR_FILENO) close(fd);
                    }
                }
                std::vector<char*> args;
                args.reserve(argv.size() + 1);
                for (const auto& a : argv) args.push_back(const_cast<char*>(a.c_str()));
                args.push_back(nullptr);
                execvp(args[0], args.data());
                _exit(127);
            }
            int status = 0;
            while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) {
                    std::cerr << "nexus: failed while waiting for '" << argv[0] << "': "
                        << std::strerror(errno) << "\n";
                    return 127;
                }
            }
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return 1;
#endif
        }

        void usage() {

            std::cout
            << "Nexus " << kVersion << "\n"
            << "Usage: nexus <command> [options]\n\n"
            << "Project commands:\n"
            << "  new <project> [--template cli|app|game|3d|paint|server|library]\n"
            << "  init [--template cli|app|game|3d|paint|server|library]\n"
            << "  info\n"
            << "  assets\n"
            << "  clean\n"
            << "  doctor [--json]\n"
            << "  self-test\n"
            << "  explain-backend <file.nx> [--target TRIPLE]\n"
            << "  editor [--gui]\n\n"
            << "Build commands:\n"
            << "  build [file.nx] [-o output] [--target TRIPLE] [--backend auto|native|llvm] [--emit-ir] [--release]\n"
            << "  run [file.nx] [--target TRIPLE] [--backend auto|native|llvm] [--release]\n"
            << "  export [--target TRIPLE] [--backend auto|native|llvm] [-o output]\n"
            << "  check [file.nx]\n"
            << "  targets\n"
            << "  fmt [file.nx] [--check]\n"
            << "  lint [file.nx]\n"
            << "  doc\n"
            << "  bench [file.nx] [--runs N]\n"
            << "  test\n"
            << "  pkg <init|add|remove|list|update|build|publish> [args]\n"
            << "  lsp\n"
            << "  version\n"
            << "  help\n\n"
            << "Backends:\n"
            << "  auto     use the builtin native backend when the program fits\n"
            << "           its subset, otherwise the LLVM backend (default)\n"
            << "  native   builtin direct machine-code backend, no external\n"
            << "           compiler or linker (x86-64 Windows/Linux subset)\n"
            << "  llvm     LLVM IR + external C++ compiler (full language)\n\n"
            << "Examples:\n"
            << "  nexus new NeonGame --template game\n"
            << "  cd NeonGame && nexus build && nexus run\n"
            << "  nexus check src/main.nx\n"
            << "  nexus build src/main.nx -o app --target x86_64-linux\n";

        }

        std::string targetTriple(const std::string& t) {

            if (t.empty() || t == "native") return {
            };

            if (t == "x86_64-linux") return "x86_64-unknown-linux-gnu";

            if (t == "aarch64-linux") return "aarch64-unknown-linux-gnu";

            if (t == "x86_64-windows") return "x86_64-pc-windows-gnu";

            if (t == "aarch64-windows") return "aarch64-pc-windows-msvc";

            if (t == "x86_64-macos") return "x86_64-apple-darwin";

            if (t == "aarch64-macos") return "arm64-apple-darwin";

            if (t == "aarch64-android") return "aarch64-linux-android";

            if (t == "wasm32-wasi") return "wasm32-wasi";

            return t;

        }

        void appendProgram(Program& dst, Program src) {

            dst.imports.insert(dst.imports.end(), src.imports.begin(), src.imports.end());

            for (auto& s : src.structs) dst.structs.push_back(std::move(s));

            for (auto& e : src.enums) dst.enums.push_back(std::move(e));

            for (auto& f : src.functions) dst.functions.push_back(std::move(f));

        }

        Program loadProgram(const fs::path& root, std::set<fs::path>& seen) {

            const auto canon = fs::weakly_canonical(root);

            if (seen.contains(canon)) return {
            };

            seen.insert(canon);
            gDiagnosticSource = canon;

            const auto src = readFile(canon);
            gDiagnosticText = src;

            auto prog = Parser(Lexer(src).tokenize()).parseProgram();

            Program all;

            for (const auto& imp : prog.imports) {

                fs::path child;

                if (imp.rfind("std.", 0) == 0) {

                    const std::string mod = imp.substr(4);

                    std::vector<fs::path> candidates;

                    candidates.push_back(canon.parent_path() / "stdlib" / (mod + ".nx"));

                    if (const char* stdDir = std::getenv("NEXUS_STD_DIR")) {
                        candidates.push_back(fs::path(stdDir) / (mod + ".nx"));
                    }

                    #ifdef NEXUS_SOURCE_ROOT
                    candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "stdlib" / (mod + ".nx"));

                    #endif
                    if (!gExecutable.empty()) {

                        const auto bin = fs::weakly_canonical(gExecutable).parent_path();

                        // Portable distribution places standard modules next to nexus.exe.
                        candidates.push_back(bin / "stdlib" / (mod + ".nx"));
                        candidates.push_back(bin / "../stdlib" / (mod + ".nx"));
                        candidates.push_back(bin / "../share/nexus/stdlib" / (mod + ".nx"));
                        candidates.push_back(bin / "../../stdlib" / (mod + ".nx"));

                    }
                    for (const auto& c : candidates) if (fs::exists(c)) {
                        child = c;
                        break;
                    }
                    if (child.empty()) throw std::runtime_error("standard module not found: " + imp);

                } else {

                    child = imp;

                    if (child.extension().empty()) child += ".nx";

                    if (child.is_relative()) {

                        const auto local = canon.parent_path() / child;

                        const auto vendorName = child.stem();

                        std::vector<fs::path> candidates;

                        candidates.push_back(local);

                        const auto pkgRoot = findProjectRoot(canon);

                        if (!pkgRoot.empty()) {

                            candidates.push_back(pkgRoot / "vendor" / vendorName / "src/main.nx");

                            candidates.push_back(pkgRoot / "vendor" / child);

                        }
                        candidates.push_back(canon.parent_path() / "vendor" / vendorName / "src/main.nx");

                        candidates.push_back(canon.parent_path() / "vendor" / child);

                        for (const auto& c : candidates) if (fs::exists(c)) {
                            child = c;
                            break;
                        }
                    }
                }
                appendProgram(all, loadProgram(child, seen));

            }
            appendProgram(all, std::move(prog));

            return all;

        }

        fs::path resolveInput(const std::string& arg) {

            if (!arg.empty()) {

                fs::path p = arg;

                if (p.is_relative()) p = fs::absolute(p);

                return p;

            }
            const ProjectConfig cfg = loadProject();

            return cfg.root / cfg.entry;

        }

        fs::path templateSource(const std::string& templ) {
            std::vector<fs::path> candidates;
            const fs::path templateName = templ + ".nx";
            const fs::path exampleName = (templ == "game" ? "neon_survivor.nx" : templ + ".nx");
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                candidates.push_back(bin / "../project-templates" / templateName);
                candidates.push_back(bin / "../share/nexus/project-templates" / templateName);
                candidates.push_back(bin / "../share/nexus/examples" / exampleName);
                candidates.push_back(bin / "../../examples/templates" / templateName);
                candidates.push_back(bin / "../../examples" / exampleName);
#ifdef NEXUS_SOURCE_ROOT
                candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "examples" / "templates" / templateName);
                candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "examples" / exampleName);
#endif
            }
            candidates.push_back(fs::current_path() / "examples" / "templates" / templateName);
            candidates.push_back(fs::current_path() / "examples" / exampleName);
            for (const auto& candidate : candidates) {
                std::error_code ec;
                if (fs::is_regular_file(candidate, ec)) return fs::weakly_canonical(candidate, ec);
            }
            return {};
        }

        fs::path runtimeSource() {
            if (const char* env = std::getenv("NEXUS_RUNTIME_DIR")) {
                fs::path candidate = fs::path(env) / "runtime.cpp";
                if (fs::is_regular_file(candidate)) return fs::weakly_canonical(candidate);
                candidate = fs::path(env) / "runtime.c";
                if (fs::is_regular_file(candidate)) return fs::weakly_canonical(candidate);
            }
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                const std::vector<fs::path> candidates = {
                    bin / "../runtime/runtime.cpp", bin / "../share/nexus/runtime/runtime.cpp",
                    bin / "runtime/runtime.cpp", bin / "../../runtime/runtime.cpp", bin / "../runtime/runtime.c"
                };
                for (const auto& candidate : candidates) {
                    std::error_code ec;
                    if (fs::is_regular_file(candidate, ec)) return fs::weakly_canonical(candidate, ec);
                }
            }
#ifdef NEXUS_SOURCE_ROOT
            const fs::path sourceRuntime = fs::path(NEXUS_SOURCE_ROOT) / "src/runtime/runtime.cpp";
            if (fs::is_regular_file(sourceRuntime)) return fs::weakly_canonical(sourceRuntime);
#endif
            const fs::path local = fs::current_path() / "runtime/runtime.cpp";
            if (fs::is_regular_file(local)) return fs::weakly_canonical(local);
#ifdef NEXUS_INSTALL_RUNTIME_DIR
            const fs::path installed = fs::path(NEXUS_INSTALL_RUNTIME_DIR) / "runtime.cpp";
            if (fs::is_regular_file(installed)) return fs::weakly_canonical(installed);
#endif
            throw std::runtime_error("Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
        }

        fs::path defaultOutput(const fs::path& input, const std::string& explicitOutput, bool release, bool windowsTarget) {

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
            if (windowsTarget && output.extension().empty()) output += ".exe";
            return output;

        }

        int launchExecutable(const fs::path& executable, const fs::path& workingDirectory) {
            #ifdef _WIN32
            const fs::path absoluteExecutable = fs::absolute(executable);
            const fs::path absoluteWorkingDirectory = fs::absolute(workingDirectory);

            // Launch directly instead of passing nested quotes through cmd.exe.
            // This also supports Unicode and spaces in both paths.
            std::wstring commandLine = L"\"" + absoluteExecutable.native() + L"\"";
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION process{};

            if (!CreateProcessW(
                    absoluteExecutable.c_str(),
                    commandLine.data(),
                    nullptr, nullptr, FALSE, 0, nullptr,
                    absoluteWorkingDirectory.c_str(),
                    &startup, &process)) {
                const DWORD error = GetLastError();
                throw std::runtime_error(
                    "cannot start native executable " + absoluteExecutable.string() +
                    " (Windows error " + std::to_string(error) + ")");
            }

            CloseHandle(process.hThread);
            const DWORD waitResult = WaitForSingleObject(process.hProcess, INFINITE);
            DWORD exitCode = 1;
            const BOOL gotExitCode = GetExitCodeProcess(process.hProcess, &exitCode);
            CloseHandle(process.hProcess);
            if (waitResult != WAIT_OBJECT_0 || !gotExitCode) {
                throw std::runtime_error("failed while waiting for the native program to exit");
            }
            return static_cast<int>(exitCode);
            #else
            return runCommand({fs::absolute(executable).string()}, workingDirectory);
            #endif
        }

        bool hostIsX64() {
#if defined(_M_X64) || defined(__x86_64__)
            return true;
#else
            return false;
#endif
        }

        // Freshly built executables are briefly locked by on-access antivirus
        // scanners on Windows; retry the write for a bounded time instead of
        // failing the build on a transient lock.
        void writeBinaryRetry(const fs::path& out, const std::uint8_t* data, std::size_t size) {
            for (int attempt = 0; ; ++attempt) {
                std::ofstream exe(out, std::ios::binary | std::ios::trunc);
                if (exe) {
                    exe.write(reinterpret_cast<const char*>(data),
                        static_cast<std::streamsize>(size));
                    exe.close();
                    if (exe) return;
                }
                if (attempt >= 20) break;
#ifdef _WIN32
                Sleep(100);
#else
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
            }
            throw std::runtime_error("cannot write " + out.string());
        }

        int compileFile(const fs::path& in, const fs::path& out, const std::string& target, bool emitIR, int opt,
            const std::string& backend = "auto") {

            std::set<fs::path> seen;

            auto prog = loadProgram(in, seen);

            SemanticAnalyzer().analyze(prog);

            if (backend != "auto" && backend != "native" && backend != "llvm")
                throw std::runtime_error("unknown backend: " + backend + " (expected auto|native|llvm)");

            // Builtin direct machine-code backend: no LLVM, Clang, assembler,
            // or linker involved. Only used for IR-independent output; with
            // --emit-ir the LLVM IR text is requested explicitly.
            if (!emitIR && backend != "llvm") {
                native::NativeTarget nativeTarget = native::NativeTarget::WindowsX64;
                bool known = native::resolveTarget(target, nativeTarget);
                if ((target.empty() || target == "native") && !hostIsX64()) known = false;
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
                            fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                            fs::perm_options::add, ec);
#endif
                        std::cout << "builtin native backend: " << image.size()
                            << " bytes -> " << out << "\n";
                        return 0;
                    }
                    if (backend == "native") throw std::runtime_error(reason);
                }
            }

            CodegenOptions cg{
                targetTriple(target), opt
            };

            const auto ir = LLVMIRGenerator(cg).generate(prog);

            fs::create_directories(out.parent_path().empty() ? fs::current_path() : out.parent_path());

            fs::path irPath = out;

            irPath += ".ll";

            std::ofstream irf(irPath, std::ios::binary);

            if (!irf) throw std::runtime_error("cannot write " + irPath.string());

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
                        if (!ec) break;
                        nativeCompiler = candidate.string();
                        break;
                    }
                }
                // On development machines and Linux/macOS package managers, PATH
                // remains a supported fallback. The official Windows portable
                // package works from its bundled backend before reaching this path.
                if (nativeCompiler.empty()) nativeCompiler = "clang++";
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
            if (!target.empty() && target != "native") backendArgv.push_back("--target=" + targetTriple(target));

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

        void writeProjectManifest(const fs::path& dir, const std::string& name, const std::string& templ) {

            std::ofstream(dir / "nexus.toml")
            << "[package]\n"
            << "name = \"" << name << "\"\n"
            << "version = \"" << kVersion << "\"\n"
            << "edition = \"2026\"\n\n"
            << "[build]\n"
            << "entry = \"src/main.nx\"\n"
            << "artifact = \"" << name << "\"\n"
            << "target = \"native\"\n\n"
            << "[window]\n"
            << "width = 1280\nheight = 720\ntitle = \"" << name << "\"\n\n"
            << "[assets]\ntextures = \"assets/textures\"\naudio = \"assets/audio\"\nmodels = \"assets/models\"\nfonts = \"assets/fonts\"\nscenes = \"scenes\"\n\n"
            << "[project]\ntemplate = \"" << templ << "\"\n";

        }

        fs::path sourceDataRoot() {

            #ifdef NEXUS_SOURCE_ROOT
            const fs::path sourceRoot = NEXUS_SOURCE_ROOT;

            if (fs::exists(sourceRoot / "examples/assets") || fs::exists(sourceRoot / "examples/templates")) return sourceRoot;

            #endif
            if (!gExecutable.empty()) {

                const auto bin = fs::weakly_canonical(gExecutable).parent_path();

                const fs::path candidates[] = {
                    fs::current_path(), bin, bin / "..", bin / "../..", bin / "../share/nexus"
                };

                for (const auto& p : candidates) {

                    if (fs::exists(p / "examples/assets") || fs::exists(p / "assets") || fs::exists(p / "examples/templates")) return fs::weakly_canonical(p);

                }
            }
            return fs::current_path();

        }

        void seedAssets(const fs::path& dir) {

            const fs::path sourceRoot = sourceDataRoot();

            const fs::path assetRoot = fs::exists(sourceRoot / "examples/assets")
            ? sourceRoot / "examples/assets"
            : sourceRoot / "assets";

            const std::vector<std::pair<fs::path, fs::path>> assets = {

                {
                    assetRoot / "beep.wav", dir / "assets/audio/beep.wav"
                },
                {
                    assetRoot / "cube.obj", dir / "assets/models/cube.obj"
                },
                {
                    assetRoot / "sprite_sheet.bmp", dir / "assets/textures/sprite_sheet.bmp"
                }
            };

            for (const auto& [src, dst] : assets) {

                if (fs::exists(src)) {

                    std::error_code ec;

                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);

                }
            }
        }

        int initProject(const fs::path& dir, const std::string& name, const std::string& templ, bool failIfNonEmpty) {

            if (fs::exists(dir / "nexus.toml")) {

                if (failIfNonEmpty) throw std::runtime_error("project already exists: " + dir.string());

            } else if (failIfNonEmpty && fs::exists(dir) && !fs::is_empty(dir)) {

                throw std::runtime_error("directory is not empty: " + dir.string());

            }
            fs::create_directories(dir / "src");

            fs::create_directories(dir / "assets/textures");

            fs::create_directories(dir / "assets/audio");

            fs::create_directories(dir / "assets/models");

            fs::create_directories(dir / "assets/fonts");

            fs::create_directories(dir / "scenes");

            const fs::path templatePath = templateSource(templ);

            const std::string source = templatePath.empty()
            ? "fn main() { print(\"Hello from NEXUS!\") }\n"
            : readFile(templatePath);

            if (!fs::exists(dir / "src/main.nx")) std::ofstream(dir / "src/main.nx") << source;

            if (!fs::exists(dir / "scenes/main.nxs")) {

                std::ofstream(dir / "scenes/main.nxs") << "NXS1\nscene = Main\nversion = 1\n";

            }
            writeProjectManifest(dir, name.empty() ? dir.filename().string() : name, templ);

            seedAssets(dir);

            std::ofstream(dir / ".gitignore") << "build/\n*.ll\n*.o\n.cache/\n";

            std::ofstream(dir / "README.md")
            << "# " << (name.empty() ? dir.filename().string() : name) << "\n\n"
            << "Generated by NEXUS " << kVersion << " (`" << templ << "` template).\n\n"
            << "Build: `nexus build`\n"
            << "Run: `nexus run`\n"
            << "Check: `nexus check`\n"
            << "Assets: `nexus assets`\n"
            << "Info: `nexus info`\n";

            return 0;

        }

        int commandInfo() {

            const auto cfg = loadProject();

            std::cout << "NEXUS project\n"
            << "  name:    " << cfg.name << "\n"
            << "  version: " << cfg.version << "\n"
            << "  root:    " << cfg.root << "\n"
            << "  entry:   " << cfg.entry << "\n"
            << "  artifact:" << cfg.artifact << "\n"
            << "  target:  " << cfg.target << "\n"
            << "  window:  " << cfg.windowWidth << "x" << cfg.windowHeight << "\n"
            << "  title:   " << cfg.windowTitle << "\n";

            return 0;

        }

        int commandAssets() {

            const auto cfg = loadProject();

            const std::vector<fs::path> dirs = {

                cfg.root / "assets/textures", cfg.root / "assets/audio", cfg.root / "assets/models",
                cfg.root / "assets/fonts", cfg.root / "scenes"
            };

            std::size_t count = 0;

            std::cout << "Assets for " << cfg.name << ":\n";

            for (const auto& dir : dirs) {

                if (!fs::exists(dir)) continue;

                for (const auto& e : fs::recursive_directory_iterator(dir)) {

                    if (!e.is_regular_file()) continue;

                    ++count;

                    std::cout << "  " << fs::relative(e.path(), cfg.root).string() << "\n";

                }
            }
            std::cout << "Total: " << count << " file(s)\n";

            return 0;

        }

        int commandClean() {

            const auto cfg = loadProject();

            std::error_code ec;

            fs::remove_all(cfg.root / "build", ec);

            if (ec) throw std::runtime_error("cannot remove build directory: " + ec.message());

            std::cout << "clean: OK\n";

            return 0;

        }

        int commandDoctor(bool json = false) {
            // Mirror compileFile's compiler discovery order so portable builds
            // with a bundled wrapper are not reported as broken.
            std::vector<fs::path> compilerCandidates;
            if (const char* configured = std::getenv("NEXUS_CXX"); configured && *configured)
                compilerCandidates.emplace_back(configured);
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                for (const auto& rel : {
                    fs::path("clang++.cmd"), fs::path("clang++.exe"), fs::path("clang++"),
                    fs::path("../bin/clang++.cmd"), fs::path("../bin/clang++.exe"),
                    fs::path("../bin/clang++"), fs::path("../toolchain/clang++.cmd")
                }) compilerCandidates.push_back(bin / rel);
            }
            compilerCandidates.emplace_back("clang++");

            std::string selectedCompiler;
            int compilerStatus = 127;
            for (const auto& candidate : compilerCandidates) {
                std::error_code ec;
                if (candidate.has_parent_path() && !fs::exists(candidate, ec)) continue;
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
                if (fs::exists(candidate / "io.nx")) stdlib = candidate;
            }
            if (stdlib.empty() && !gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                for (const auto& candidate : {bin / "../stdlib", bin / "stdlib", bin / "../share/nexus/stdlib"}) {
                    if (fs::is_regular_file(candidate / "io.nx")) {
                        stdlib = fs::weakly_canonical(candidate);
                        break;
                    }
                }
            }
#ifdef NEXUS_SOURCE_ROOT
            if (stdlib.empty() && fs::is_regular_file(fs::path(NEXUS_SOURCE_ROOT) / "stdlib" / "io.nx"))
                stdlib = fs::path(NEXUS_SOURCE_ROOT) / "stdlib";
#endif

            const fs::path project = findProjectRoot(fs::current_path());
            const bool runtimeOk = !runtime.empty() && fs::exists(runtime);
            const bool templatesOk = !templateFile.empty() && fs::exists(templateFile);
            const bool stdlibOk = !stdlib.empty();
            const std::string compilerLabel = compilerStatus == 0 ? selectedCompiler : std::string();

#if defined(_WIN32) || defined(__linux__)
            const bool nativeHostSupported = hostIsX64();
#else
            const bool nativeHostSupported = false;
#endif
            if (json) {
                std::cout << "{\n"
                          << "  \"version\": \"" << kVersion << "\",\n"
                          << "  \"native_backend\": " << (nativeHostSupported ? "true" : "false") << ",\n"
                          << "  \"llvm_backend\": " << (compilerStatus == 0 ? "true" : "false") << ",\n"
                          << "  \"compiler_found\": " << (compilerStatus == 0 ? "true" : "false") << ",\n"
                          << "  \"runtime\": " << (runtimeOk ? "true" : "false") << ",\n"
                          << "  \"stdlib\": " << (stdlibOk ? "true" : "false") << ",\n"
                          << "  \"templates\": " << (templatesOk ? "true" : "false") << ",\n"
                          << "  \"project\": " << (!project.empty() ? "true" : "false") << "\n"
                          << "}\n";
            } else {
                std::cout << "NEXUS doctor " << kVersion << "\n"
                          << "  executable:     " << (gExecutable.empty() ? "unknown" : gExecutable.string()) << "\n"
                          << "  native backend: " << (nativeHostSupported ? "OK (x86-64 built-in backend)" : "unsupported on this host") << "\n"
                          << "  LLVM backend:   " << (compilerStatus == 0 ? "OK (" + compilerLabel + ")" : "MISSING") << "\n"
                          << "  runtime:        " << (runtimeOk ? runtime.string() : "MISSING") << "\n"
                          << "  stdlib:         " << (stdlibOk ? stdlib.string() : "MISSING") << "\n"
                          << "  templates:      " << (templatesOk ? templateFile.string() : "MISSING") << "\n"
                          << "  project:        " << (project.empty() ? "not in a project" : project.string()) << "\n";
            }

            if (compilerStatus != 0 && !json)
                std::cout << "note: the built-in native backend handles only its supported subset; other programs need clang++ or NEXUS_CXX.\n";
            if (!runtimeOk && !json)
                std::cerr << "warning: runtime.cpp/runtime.c was not found; LLVM fallback builds may fail.\n";
            if (!templatesOk && !json)
                std::cerr << "warning: project templates were not found; the new command may fail.\n";

            // LLVM is optional for programs supported by the built-in backend.
            return 0;
        }

        int commandExplainBackend(const fs::path& file, const std::string& target) {
            std::set<fs::path> seen;
            auto program = loadProgram(file, seen);
            SemanticAnalyzer().analyze(program);

            const std::string selectedTarget = target.empty() ? "native" : target;
            native::NativeTarget nativeTarget = native::NativeTarget::WindowsX64;
            bool targetSupported = native::resolveTarget(selectedTarget, nativeTarget);
            if (selectedTarget == "native" && !hostIsX64()) targetSupported = false;

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

        void writeText(const fs::path& p, const std::string& data) {

            fs::create_directories(p.parent_path().empty() ? fs::current_path() : p.parent_path());

            std::ofstream f(p, std::ios::binary);

            if (!f) throw std::runtime_error("cannot write " + p.string());

            f << data;

        }

        int commandFmt(const fs::path& file, bool checkOnly = false) {
            const std::string src = readFile(file);
            const std::string formatted = formatNexusSource(src);
            const bool touched = formatted != src;
            if (touched && checkOnly) {
                std::cout << "fmt: needs formatting -> " << file << '\n';
                return 1;
            }
            if (touched) writeText(file, formatted);
            if (checkOnly) std::cout << "fmt: OK -> " << file << '\n';
            else std::cout << "fmt: " << (touched ? "updated" : "already formatted") << " -> " << file << '\n';
            return 0;
        }

        int commandSelfTest() {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path scratch = fs::temp_directory_path() /
                ("nexus-self-test-" + std::to_string(stamp));
            const fs::path source = scratch / "self_test.nx";
            fs::path executable = scratch / "self_test";
#ifdef _WIN32
            executable += ".exe";
#endif
#if defined(_WIN32) || defined(__linux__)
            const bool preferNativeBackend = hostIsX64();
#else
            const bool preferNativeBackend = false;
#endif
            const std::string backend = preferNativeBackend ? "native" : "auto";
            std::error_code ec;
            fs::create_directories(scratch, ec);
            if (ec) throw std::runtime_error("cannot create self-test directory: " + ec.message());

            try {
                writeText(source, "fn main() -> i64 { return 42 }\n");
                const int buildRc = compileFile(source, executable, "native", false, 2, backend);
                if (buildRc != 0) throw std::runtime_error("compiler returned " + std::to_string(buildRc));
                const int runRc = runCommand({executable.string()});
                if (runRc != 42) throw std::runtime_error("self-test expected exit code 42 but got " + std::to_string(runRc));
                fs::remove_all(scratch, ec);
                std::cout << "self-test: PASS (" <<
                    (preferNativeBackend ? "built-in native" : "configured compiler fallback") <<
                    " compile + run)\n";
                return 0;
            } catch (...) {
                fs::remove_all(scratch, ec);
                throw;
            }
        }

        int commandLint(const fs::path& file) {

            const std::string src = readFile(file);

            std::size_t warnings = 0, lineNo = 0;

            std::istringstream in(src);

            std::string line;

            while (std::getline(in,line)) {

                ++lineNo;

                if (!line.empty() && (line.back()==' ' || line.back()=='\t' || line.back()=='\r')) {

                    ++warnings;
                    std::cout << "warning[NX9001]: trailing whitespace at " << file << ':' << lineNo << '\n';

                }
                if (line.find("TODO") != std::string::npos || line.find("FIXME") != std::string::npos) {

                    ++warnings;
                    std::cout << "warning[NX9002]: unfinished marker at " << file << ':' << lineNo << '\n';

                }
            }
            std::set<fs::path> seen;

            auto prog = loadProgram(file, seen);

            SemanticAnalyzer().analyze(prog);

            std::cout << "lint: OK" << (warnings ? ", " + std::to_string(warnings) + " warning(s)" : "") << '\n';

            return 0;

        }

        int commandDoc() {

            const auto cfg = loadProject();

            const auto entry = cfg.root / cfg.entry;

            const std::string src = readFile(entry);

            std::istringstream in(src);

            std::ostringstream docs;

            docs << "# " << cfg.name << " API\n\nGenerated by NEXUS 1.1.0.\n\n";

            std::string pending;

            std::string line;

            while (std::getline(in,line)) {

                const auto t = trim(line);

                if (t.rfind("///",0)==0) {
                    pending += trim(t.substr(3)) + "\n";
                    continue;
                }
                if (t.rfind("fn ",0)==0 || t.rfind("struct ",0)==0 || t.rfind("enum ",0)==0) {

                    docs << "## " << t << "\n\n";

                    if (!pending.empty()) {
                        docs << pending << "\n";
                        pending.clear();
                    }
                }
            }
            const fs::path out = cfg.root / "docs/API.md";

            writeText(out, docs.str());

            std::cout << "doc: generated " << out << '\n';

            return 0;

        }

        int commandBench(const fs::path& file, int runs) {

            const auto out = fs::temp_directory_path() / (file.stem().string() + ".nexus-bench");

            compileFile(file, out, "native", false, 3);

            std::vector<long long> samples;

            samples.reserve(std::max(1,runs));

            for (int i=0;i<std::max(1,runs);++i) {

                const auto a = std::chrono::steady_clock::now();

                const int rc = runCommand({out.string()});

                const auto b = std::chrono::steady_clock::now();

                if (rc != 0) throw std::runtime_error("benchmark program exited with code " + std::to_string(rc));

                samples.push_back(std::chrono::duration_cast<std::chrono::microseconds>(b-a).count());

            }
            std::error_code ec;
            fs::remove(out,ec);

            long long total=0;
            for(auto x:samples) total+=x;

            std::cout << "bench: " << samples.size() << " run(s), average " << (total / samples.size()) << " us\n";

            return 0;

        }

        int commandTest() {

            const auto cfg = loadProject();

            const auto root = cfg.root / "tests/programs";

            if (!fs::exists(root)) {
                std::cout << "test: no tests/programs directory\n";
                return 0;
            }
            std::size_t count=0;

            for (const auto& e : fs::recursive_directory_iterator(root)) {

                if (!e.is_regular_file() || e.path().extension() != ".nx") continue;

                ++count;

                const auto out = fs::temp_directory_path() / (e.path().stem().string()+".nexus-test");

                compileFile(e.path(),out,"native",false,2);

                const int rc=runCommand({out.string()});

                std::error_code ec;
                fs::remove(out,ec);

                if(rc!=0) throw std::runtime_error("test failed: "+e.path().string());

                std::cout << "  PASS " << fs::relative(e.path(),cfg.root).string() << "\n";

            }
            std::cout << "test: " << count << " program(s) passed\n";

            return 0;

        }

        int commandExport(const std::vector<std::string>& args) {
            const auto cfg = loadProject();
            std::string target = cfg.target;
            std::string backend = "auto";
            std::string explicitOutput;
            for (std::size_t i = 0; i < args.size(); ++i) {
                const std::string& a = args[i];
                if (a.rfind("--target=", 0) == 0) target = a.substr(9);
                else if (a == "--target" && i + 1 < args.size()) target = args[++i];
                else if (a.rfind("--backend=", 0) == 0) backend = a.substr(10);
                else if (a == "--backend" && i + 1 < args.size()) backend = args[++i];
                else if (a == "-o" && i + 1 < args.size()) explicitOutput = args[++i];
                else throw std::runtime_error("unknown option: " + a);
            }
            const auto in = cfg.root / cfg.entry;
            const std::string resolvedTriple = targetTriple(target);
            bool windowsTarget = false;
#ifdef _WIN32
            windowsTarget = target.empty() || target == "native" ||
                resolvedTriple.find("windows") != std::string::npos;
#else
            windowsTarget = resolvedTriple.find("windows") != std::string::npos;
#endif
            const auto dist = cfg.root / "dist";
            fs::create_directories(dist);
            fs::path out;
            if (!explicitOutput.empty()) {
                out = fs::path(explicitOutput);
                if (out.is_relative()) out = dist / out;
            } else {
                out = dist / cfg.artifact;
            }
            if (windowsTarget && out.extension().empty()) out += ".exe";
            compileFile(in, out, target, false, 3, backend);

            const std::string label = target.empty() ? "native" : target;
            const auto stage = dist / (cfg.name + "-" + cfg.version);
            std::error_code ec;
            fs::remove_all(stage, ec);
            fs::create_directories(stage, ec);
            fs::copy(out, stage / out.filename(),
                fs::copy_options::overwrite_existing, ec);
            if (ec) throw std::runtime_error("cannot stage export binary: " + ec.message());
            for (const auto& rel : {std::string("assets"), std::string("scenes")}) {
                if (fs::exists(cfg.root / rel))
                    fs::copy(cfg.root / rel, stage / rel,
                        fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            }
            if (fs::exists(cfg.root / "nexus.toml"))
                fs::copy(cfg.root / "nexus.toml", stage / "nexus.toml",
                    fs::copy_options::overwrite_existing, ec);
            if (ec) throw std::runtime_error("cannot stage export files: " + ec.message());
            writeText(stage / "EXPORT.txt",
                "package = \"" + cfg.name + "\"\nversion = \"" + cfg.version +
                "\"\ntarget = \"" + label + "\"\nbackend = \"" + backend + "\"\n");

            const auto archive = dist / (cfg.name + "-" + cfg.version + "-" + label + ".zip");
            fs::remove(archive, ec);
            // cmake is only needed to zip the archive; the binary and the
            // staged directory are the actual deliverables. Clean machines
            // without cmake still get a successful export.
            const int rc = runCommand(
                {"cmake", "-E", "tar", "cf", archive.string(), "--format=zip", stage.filename().string()},
                dist);
            if (rc == 127) {
                std::cout << "export: binary  -> " << out << "\n"
                    << "export: staged  -> " << stage << "\n"
                    << "export: note: archive skipped (cmake not on PATH)\n";
                return 0;
            }
            if (rc != 0) throw std::runtime_error("cannot create export archive");
            std::cout << "export: binary  -> " << out << "\n"
                << "export: archive -> " << archive << "\n";
            return 0;
        }

        int commandEditor() {

            const auto cfg = loadProject();

            const auto vscode = cfg.root / ".vscode";

            fs::create_directories(vscode);

            writeText(vscode / "settings.json", R"JSON({
  "files.associations": { "*.nx": "nexus" },
  "editor.tabSize": 4,
  "editor.insertSpaces": true
}
)JSON");

            writeText(vscode / "tasks.json", R"JSON({
  "version": "2.0.0",
  "tasks": [{
    "label": "NEXUS Build",
    "type": "shell",
    "command": "nexus build --release",
    "group": "build"
  }]
}
)JSON");

            std::cout << "editor: generated VS Code integration in .vscode/\n";

            std::cout << "editor: use `nexus lsp` as the language-server command\n";

            return 0;

        }

        bool validPackageName(const std::string& name) {
            if (name.empty() || name == "." || name == "..") return false;
            for (const unsigned char ch : name) {
                if (!std::isalnum(ch) && ch != '_' && ch != '-') return false;
            }
            return true;
        }

        void updateDependency(const fs::path& manifest, const std::string& name,
                const std::string& value, bool remove) {
            std::istringstream input(readFile(manifest));
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(input, line)) lines.push_back(line);
            std::size_t start = lines.size(), finish = lines.size();
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (trim(lines[i]) != "[dependencies]") continue;
                start = i + 1;
                for (std::size_t j = i + 1; j < lines.size(); ++j) {
                    const std::string t = trim(lines[j]);
                    if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
                        finish = j;
                        break;
                    }
                }
                break;
            }
            if (start == lines.size()) {
                if (remove) return;
                if (!lines.empty() && !trim(lines.back()).empty()) lines.emplace_back();
                lines.emplace_back("[dependencies]");
                lines.emplace_back(name + " = \"" + value + "\"");
            } else {
                bool found = false;
                std::size_t at = start;
                for (; at < finish; ++at) {
                    const std::string t = trim(lines[at]);
                    if (t.empty() || t.front() == '#') continue;
                    const auto eq = t.find('=');
                    if (eq == std::string::npos || trim(t.substr(0, eq)) != name) continue;
                    found = true;
                    break;
                }
                if (found) {
                    if (remove) lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(at));
                    else lines[at] = name + " = \"" + value + "\"";
                } else if (!remove) {
                    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(finish),
                        name + " = \"" + value + "\"");
                }
            }
            std::ostringstream output;
            for (const auto& entry : lines) output << entry << '\n';
            writeText(manifest, output.str());
        }

        void refreshLockfile(const ProjectConfig& cfg) {
            std::vector<std::pair<std::string, std::string>> dependencies;
            std::istringstream input(readFile(cfg.root / "nexus.toml"));
            std::string line;
            bool inDependencies = false;
            while (std::getline(input, line)) {
                const std::string t = trim(line);
                if (t.empty() || t.front() == '#') continue;
                if (t.front() == '[' && t.back() == ']') {
                    inDependencies = t == "[dependencies]";
                    continue;
                }
                if (!inDependencies) continue;
                const auto eq = t.find('=');
                if (eq == std::string::npos) continue;
                const std::string name = trim(t.substr(0, eq));
                std::string value = trim(t.substr(eq + 1));
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);
                std::string version = value;
                const fs::path local(value);
                if (!local.is_absolute()) {
                    const fs::path dependencyRoot = (cfg.root / local).lexically_normal();
                    const fs::path rel = dependencyRoot.lexically_relative(cfg.root);
                    std::error_code ec;
                    if (!rel.empty() && *rel.begin() != ".." &&
                            fs::is_regular_file(dependencyRoot / "nexus.toml", ec)) {
                        try {
                            const auto dep = loadProject(dependencyRoot);
                            if (!dep.version.empty()) version = dep.version;
                        } catch (const std::exception&) {}
                    }
                }
                dependencies.emplace_back(name, version);
            }
            std::ostringstream lock;
            lock << "# NEXUS lockfile v1\npackage = \"" << cfg.name
                 << "\"\nversion = \"" << cfg.version << "\"\n";
            if (!dependencies.empty()) {
                lock << "\n[dependencies]\n";
                for (const auto& dep : dependencies) lock << dep.first << " = \"" << dep.second << "\"\n";
            }
            writeText(cfg.root / "nexus.lock", lock.str());
        }

        void copyPackageTree(const fs::path& source, const fs::path& destination) {
            const std::set<std::string> skipped = {
                ".git", ".vscode", "build", "dist", "node_modules", "CMakeFiles"
            };
            std::error_code ec;
            fs::create_directories(destination, ec);
            if (ec) throw std::runtime_error("cannot create vendor directory: " + ec.message());
            try {
                for (fs::recursive_directory_iterator it(source, fs::directory_options::skip_permission_denied, ec),
                        end; it != end; it.increment(ec)) {
                    if (ec) throw std::runtime_error("cannot traverse package source: " + ec.message());
                    const auto status = it->symlink_status(ec);
                    if (ec) throw std::runtime_error("cannot inspect package entry: " + ec.message());
                    if (fs::is_symlink(status)) continue;
                    if (fs::is_directory(status) && skipped.contains(it->path().filename().string())) {
                        it.disable_recursion_pending();
                        continue;
                    }
                    const fs::path rel = it->path().lexically_relative(source);
                    const fs::path target = destination / rel;
                    if (fs::is_directory(status)) fs::create_directories(target, ec);
                    else if (fs::is_regular_file(status)) {
                        fs::create_directories(target.parent_path(), ec);
                        if (!ec) fs::copy_file(it->path(), target, fs::copy_options::overwrite_existing, ec);
                    }
                    if (ec) throw std::runtime_error("cannot copy package entry '" + rel.string() + "': " + ec.message());
                }
            } catch (...) {
                std::error_code cleanup;
                fs::remove_all(destination, cleanup);
                throw;
            }
        }

        int commandPkg(const std::vector<std::string>& args) {
            const auto cfg = loadProject();
            if (args.empty() || args[0] == "help") {
                std::cout << "nexus pkg init|add <path>|remove <name>|list|update|build|publish\n";
                return 0;
            }
            const std::string sub = args[0];
            const fs::path vendor = cfg.root / "vendor";
            if (sub == "init") {
                fs::create_directories(vendor);
                refreshLockfile(cfg);
                std::cout << "pkg init: OK\n";
                return 0;
            }
            if (sub == "list") {
                std::cout << "Packages for " << cfg.name << ":\n";
                if (fs::exists(vendor)) {
                    for (const auto& entry : fs::directory_iterator(vendor)) {
                        if (!entry.is_directory()) continue;
                        std::cout << "  " << entry.path().filename().string();
                        if (fs::is_regular_file(entry.path() / "nexus.toml")) {
                            try { std::cout << " " << loadProject(entry.path()).version; }
                            catch (const std::exception&) { std::cout << " (invalid manifest)"; }
                        }
                        std::cout << '\n';
                    }
                }
                return 0;
            }
            if (sub == "add") {
                if (args.size() != 2) throw std::runtime_error("pkg add requires exactly one local package directory");
                fs::path source(args[1]);
                if (source.is_relative()) source = fs::absolute(source);
                std::error_code ec;
                source = fs::weakly_canonical(source, ec);
                if (ec || !fs::is_directory(source, ec))
                    throw std::runtime_error("pkg add source is not an accessible directory");
                if (!fs::is_regular_file(source / "nexus.toml"))
                    throw std::runtime_error("package directory must contain nexus.toml");
                const auto dep = loadProject(source);
                if (!validPackageName(dep.name))
                    throw std::runtime_error("invalid package name: use letters, digits, '_' or '-'");
                const fs::path root = fs::weakly_canonical(cfg.root, ec);
                const fs::path destination = (vendor / dep.name).lexically_normal();
                if (source == root) throw std::runtime_error("cannot add the current project as its own dependency");
                const fs::path relativeDestination = destination.lexically_relative(source);
                if (!relativeDestination.empty() && *relativeDestination.begin() != "..")
                    throw std::runtime_error("package source contains the vendor destination; choose a separate directory");
                fs::remove_all(destination, ec);
                if (ec) throw std::runtime_error("cannot replace vendored package: " + ec.message());
                copyPackageTree(source, destination);
                try {
                    updateDependency(cfg.root / "nexus.toml", dep.name, "vendor/" + dep.name, false);
                    refreshLockfile(cfg);
                } catch (...) {
                    std::error_code cleanup;
                    fs::remove_all(destination, cleanup);
                    throw;
                }
                std::cout << "pkg add: vendored " << dep.name << " " << dep.version << "\n";
                return 0;
            }
            if (sub == "remove") {
                if (args.size() != 2) throw std::runtime_error("pkg remove requires exactly one package name");
                const std::string name = args[1];
                if (!validPackageName(name))
                    throw std::runtime_error("invalid package name: path separators and traversal are not allowed");
                std::error_code ec;
                fs::remove_all(vendor / name, ec);
                if (ec) throw std::runtime_error("cannot remove vendored package: " + ec.message());
                updateDependency(cfg.root / "nexus.toml", name, "", true);
                refreshLockfile(cfg);
                std::cout << "pkg remove: removed " << name << " and refreshed nexus.toml/nexus.lock\n";
                return 0;
            }
            if (sub == "update") {
                refreshLockfile(cfg);
                std::cout << "pkg update: lockfile refreshed from nexus.toml\n";
                return 0;
            }
            if (sub == "build") {
                const auto input = cfg.root / cfg.entry;
                const std::string triple = targetTriple(cfg.target);
                bool windowsTarget = false;
#ifdef _WIN32
                windowsTarget = cfg.target.empty() || cfg.target == "native" || triple.find("windows") != std::string::npos;
#else
                windowsTarget = triple.find("windows") != std::string::npos;
#endif
                const auto output = defaultOutput(input, "", true, windowsTarget);
                compileFile(input, output, cfg.target, false, 3);
                std::cout << "pkg build: OK -> " << output << "\n";
                return 0;
            }
            if (sub == "publish") {
                if (!validPackageName(cfg.name)) throw std::runtime_error("invalid package name in nexus.toml");
                const auto dist = cfg.root / "dist";
                fs::create_directories(dist);
                const auto archive = dist / (cfg.name + "-" + cfg.version + ".nxpkg");
                std::vector<std::string> tarArgv = {"cmake", "-E", "tar", "cf", archive.string(), "--format=zip"};
                for (const auto& relative : {std::string("nexus.toml"), std::string("src"),
                        std::string("assets"), std::string("scenes"), std::string("vendor"), std::string("docs")})
                    if (fs::exists(cfg.root / relative)) tarArgv.push_back(relative);
                const int rc = runCommand(tarArgv, cfg.root);
                if (rc != 0) throw std::runtime_error("cannot create package archive");
                std::cout << "pkg publish: created local archive " << archive
                          << " (no remote registry upload is configured)\n";
                return 0;
            }
            throw std::runtime_error("unknown pkg command: " + sub);
        }

    int commandLsp() {
        const auto extractString = [](const std::string& json, const std::string& field) {
            const std::string key = "\"" + field + "\"";
            auto pos = json.find(key);
            if (pos == std::string::npos) return std::string{};
            pos = json.find(':', pos + key.size());
            if (pos == std::string::npos) return std::string{};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
            if (pos >= json.size() || json[pos] != '"') return std::string{};
            std::string value;
            for (++pos; pos < json.size(); ++pos) {
                const char c = json[pos];
                if (c == '"') return value;
                if (c == '\\' && pos + 1 < json.size()) {
                    const char escaped = json[++pos];
                    switch (escaped) {
                        case 'n': value.push_back('\n'); break;
                        case 'r': value.push_back('\r'); break;
                        case 't': value.push_back('\t'); break;
                        default: value.push_back(escaped); break;
                    }
                } else {
                    value.push_back(c);
                }
            }
            return std::string{};
        };
        const auto extractId = [](const std::string& json) {
            const auto key = json.find("\"id\"");
            if (key == std::string::npos) return std::string{};
            auto pos = json.find(':', key + 4);
            if (pos == std::string::npos) return std::string{};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
            if (pos >= json.size()) return std::string{};
            const auto begin = pos;
            if (json[pos] == '"') {
                ++pos;
                bool escaped = false;
                while (pos < json.size()) {
                    if (!escaped && json[pos] == '"') { ++pos; break; }
                    if (!escaped && json[pos] == '\\') escaped = true;
                    else escaped = false;
                    ++pos;
                }
            } else {
                while (pos < json.size() && json[pos] != ',' && json[pos] != '}' &&
                    !std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
            }
            return json.substr(begin, pos - begin);
        };
        const auto extractNumber = [](const std::string& json, const std::string& field) {
            const std::string key = "\"" + field + "\"";
            const auto keyPos = json.find(key);
            if (keyPos == std::string::npos) return std::size_t{0};
            auto pos = json.find(':', keyPos + key.size());
            if (pos == std::string::npos) return std::size_t{0};
            ++pos;
            while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
            const auto begin = pos;
            while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) ++pos;
            if (begin == pos) return std::size_t{0};
            try { return static_cast<std::size_t>(std::stoull(json.substr(begin, pos - begin))); }
            catch (const std::exception&) { return std::size_t{0}; }
        };
        const auto send = [](const std::string& payload) {
            std::cout << "Content-Length: " << payload.size() << "\r\n\r\n"
                      << payload << std::flush;
        };
        const auto jsonEscape = [](const std::string& value) {
            std::string escaped;
            for (const unsigned char c : value) {
                switch (c) {
                    case '"': escaped += "\\\""; break;
                    case '\\': escaped += "\\\\"; break;
                    case '\n': escaped += "\\n"; break;
                    case '\r': escaped += "\\r"; break;
                    case '\t': escaped += "\\t"; break;
                    case '\b': escaped += "\\b"; break;
                    case '\f': escaped += "\\f"; break;
                    default:
                        if (c < 0x20) {
                            constexpr char hex[] = "0123456789abcdef";
                            escaped += "\\u00";
                            escaped.push_back(hex[(c >> 4) & 0x0f]);
                            escaped.push_back(hex[c & 0x0f]);
                        } else escaped.push_back(static_cast<char>(c));
                        break;
                }
            }
            return escaped;
        };
        struct LspDiagnostic {
            std::size_t line = 0;
            std::size_t character = 0;
            std::string code;
            std::string message;
        };
        const auto collectDiagnostics = [](const std::string& source) {
            std::vector<LspDiagnostic> diagnostics;
            const auto add = [&](const std::exception& error, const std::string& code) {
                LspDiagnostic diagnostic;
                diagnostic.code = code;
                const std::string raw = error.what();
                const auto marker = raw.rfind(" at ");
                diagnostic.message = marker == std::string::npos ? raw : trim(raw.substr(0, marker));
                if (marker != std::string::npos) {
                    std::size_t line = 0, column = 0;
                    if (std::sscanf(raw.c_str() + marker + 4, "%zu:%zu", &line, &column) == 2 &&
                        line > 0 && column > 0) {
                        diagnostic.line = line - 1;
                        diagnostic.character = column - 1;
                    } else {
                        diagnostic.message = raw;
                    }
                }
                diagnostics.push_back(std::move(diagnostic));
            };

            std::vector<Token> tokens;
            try {
                tokens = Lexer(source).tokenize();
            } catch (const std::exception& error) {
                add(error, "NX1000");
                return diagnostics;
            }
            const std::vector<Token> diagnosticTokens = tokens;
            Program program;
            try {
                program = Parser(std::move(tokens)).parseProgram();
            } catch (const std::exception& error) {
                add(error, "NX1001");
                return diagnostics;
            }
            try {
                SemanticAnalyzer().analyze(program);
            } catch (const std::exception& error) {
                add(error, "NX2001");
                const std::string message = error.what();
                const std::string prefix = "unknown variable: ";
                if (message.rfind(prefix, 0) == 0) {
                    const std::string unknownName = message.substr(prefix.size());
                    for (auto token = diagnosticTokens.rbegin(); token != diagnosticTokens.rend(); ++token) {
                        if (token->kind == TokenKind::Identifier && token->text == unknownName) {
                            diagnostics.back().line = token->pos.line > 0 ? token->pos.line - 1 : 0;
                            diagnostics.back().character = token->pos.column > 0 ? token->pos.column - 1 : 0;
                            break;
                        }
                    }
                }
            }
            return diagnostics;
        };
        std::unordered_map<std::string, std::string> openDocuments;
        // Decode the path component of a file:// URI, including UTF-8 bytes.
        const auto pathFromUri = [](const std::string& uri) -> fs::path {
            if (uri.rfind("file://", 0) != 0) return {};
            const std::string encoded = uri.substr(7);
            std::string decoded;
            decoded.reserve(encoded.size());
            const auto hexValue = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            for (std::size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] == '%' && i + 2 < encoded.size()) {
                    const int high = hexValue(encoded[i + 1]);
                    const int low = hexValue(encoded[i + 2]);
                    if (high >= 0 && low >= 0) {
                        decoded.push_back(static_cast<char>((high << 4) | low));
                        i += 2;
                        continue;
                    }
                }
                decoded.push_back(encoded[i]);
            }
#ifdef _WIN32
            if (decoded.size() >= 3 && decoded[0] == '/' &&
                ((decoded[1] >= 'A' && decoded[1] <= 'Z') ||
                 (decoded[1] >= 'a' && decoded[1] <= 'z')) && decoded[2] == ':') {
                decoded.erase(decoded.begin());
            }
#endif
            return fs::path(decoded);
        };
        const auto uriFromPath = [](const fs::path& inputPath) {
            std::error_code ec;
            fs::path absolute = fs::absolute(inputPath, ec);
            if (ec) absolute = inputPath;
            std::string path = absolute.lexically_normal().generic_string();
            if (path.empty() || path.front() != '/') path.insert(path.begin(), '/');
            static constexpr char hex[] = "0123456789ABCDEF";
            std::string encoded;
            encoded.reserve(path.size());
            for (const unsigned char ch : path) {
                const bool unreserved = (ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
                    ch == '/' || ch == ':' || ch == '-' || ch == '_' ||
                    ch == '.' || ch == '~';
                if (unreserved) {
                    encoded.push_back(static_cast<char>(ch));
                } else {
                    encoded.push_back('%');
                    encoded.push_back(hex[ch >> 4]);
                    encoded.push_back(hex[ch & 0x0f]);
                }
            }
            return std::string("file://") + encoded;
        };
        // Cache disk-backed source files per workspace. Open buffers overlay this
        // cache, so typing does not trigger a directory walk for every LSP request.
        std::map<std::string, std::map<std::string, std::string>> workspaceDiskCache;
        const auto workspaceRootForUri = [&](const std::string& uri) -> fs::path {
            const fs::path documentPath = pathFromUri(uri);
            if (documentPath.empty()) return {};
            const fs::path projectRoot = findProjectRoot(documentPath);
            return projectRoot.empty() ? documentPath.parent_path() : projectRoot;
        };
        const auto workspaceCacheKeyForUri = [&](const std::string& uri) {
            const fs::path root = workspaceRootForUri(uri);
            if (root.empty() || root == root.root_path()) return std::string{};
            std::error_code ec;
            const fs::path normalized = fs::weakly_canonical(root, ec);
            return (ec ? root.lexically_normal() : normalized).generic_string();
        };
        const auto collectWorkspaceDocuments = [&](const std::string& requestUri,
                const std::string& requestSource) {
            std::map<std::string, std::string> byUri;
            const fs::path documentPath = pathFromUri(requestUri);
            const fs::path projectRoot = documentPath.empty() ? fs::path{} : findProjectRoot(documentPath);
            const bool hasProjectManifest = !projectRoot.empty();
            const fs::path root = documentPath.empty() ? fs::path{} :
                (hasProjectManifest ? projectRoot : documentPath.parent_path());

            // Virtual editor URIs and root-level paths only participate through
            // currently open buffers. Never walk the filesystem root.
            const bool canScan = !root.empty() && root != root.root_path();
            const std::string cacheKey = canScan ? workspaceCacheKeyForUri(requestUri) : std::string{};
            if (canScan && !cacheKey.empty()) {
                auto cached = workspaceDiskCache.find(cacheKey);
                if (cached == workspaceDiskCache.end()) {
                    std::map<std::string, std::string> diskDocuments;
                    std::size_t indexedFiles = 0;
                    constexpr std::size_t maxWorkspaceFiles = 10000;
                    std::error_code ec;
                    const auto addFile = [&](const fs::path& path) {
                        if (indexedFiles >= maxWorkspaceFiles || path.extension() != ".nx") return;
                        const std::string fileUri = uriFromPath(path);
                        if (diskDocuments.find(fileUri) != diskDocuments.end()) return;
                        try {
                            diskDocuments.emplace(fileUri, readFile(path));
                            ++indexedFiles;
                        } catch (const std::exception&) {
                            // An unreadable or transiently removed file is not fatal.
                        }
                    };
                    if (hasProjectManifest) {
                        const auto options = fs::directory_options::skip_permission_denied;
                        fs::recursive_directory_iterator it(root, options, ec), end;
                        while (!ec && it != end && indexedFiles < maxWorkspaceFiles) {
                            const fs::path current = it->path();
                            if (it->is_directory(ec)) {
                                const std::string name = current.filename().string();
                                if (name == ".git" || name == "build" || name == "dist" ||
                                    name == "target" || name == "node_modules" || name == ".nexus") {
                                    it.disable_recursion_pending();
                                }
                            } else if (it->is_regular_file(ec)) {
                                addFile(current);
                            }
                            it.increment(ec);
                        }
                    } else {
                        // Without nexus.toml, discover only direct .nx siblings.
                        fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
                        while (!ec && it != end && indexedFiles < maxWorkspaceFiles) {
                            if (it->is_regular_file(ec)) addFile(it->path());
                            it.increment(ec);
                        }
                    }
                    cached = workspaceDiskCache.emplace(cacheKey, std::move(diskDocuments)).first;
                }
                byUri = cached->second;
            }

            // Unsaved editor buffers override cached disk content. Include all
            // open buffers so cross-file navigation also works for virtual docs.
            for (const auto& opened : openDocuments) byUri[opened.first] = opened.second;
            byUri[requestUri] = requestSource;

            std::vector<std::pair<std::string, std::string>> result;
            result.reserve(byUri.size());
            for (const auto& item : byUri) result.push_back(item);
            return result;
        };
        const auto utf16Length = [](const std::string& text) {
            std::size_t units = 0;
            for (std::size_t i = 0; i < text.size();) {
                const unsigned char ch = static_cast<unsigned char>(text[i]);
                std::size_t width = 1;
                if ((ch & 0xe0) == 0xc0) width = 2;
                else if ((ch & 0xf0) == 0xe0) width = 3;
                else if ((ch & 0xf8) == 0xf0) width = 4;
                if (i + width > text.size()) width = 1;
                units += width == 4 ? 2 : 1;
                i += width;
            }
            return units;
        };
        const auto byteOffsetForUtf16 = [](const std::string& text, std::size_t wanted) {
            std::size_t units = 0;
            for (std::size_t i = 0; i < text.size();) {
                const unsigned char ch = static_cast<unsigned char>(text[i]);
                std::size_t width = 1;
                if ((ch & 0xe0) == 0xc0) width = 2;
                else if ((ch & 0xf0) == 0xe0) width = 3;
                else if ((ch & 0xf8) == 0xf0) width = 4;
                if (i + width > text.size()) width = 1;
                const std::size_t step = width == 4 ? 2 : 1;
                if (units + step > wanted) return i;
                if (units + step == wanted) return i + width;
                units += step;
                i += width;
            }
            return text.size();
        };
        const auto identifierCharacter = [](unsigned char ch) {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '_';
        };
        struct LspIdentifierOccurrence {
            std::string uri;
            std::size_t line = 0;
            std::size_t startCharacter = 0;
            std::size_t endCharacter = 0;
            bool isDeclaration = false;
        };

        // The LSP keeps per-file lexical indexes and rebuilds only files whose
        // source text changed. These are intentionally syntax/lexical indexes,
        // not substitutes for semantic analysis in the compiler.
        struct LspScopeInfo {
            std::size_t parent = static_cast<std::size_t>(-1);
            std::size_t depth = 0;
        };
        struct LspSymbolDecl {
            std::string name;
            std::string identity;
            std::string kind;
            std::size_t tokenIndex = 0;
            std::size_t visibleFrom = 0;
            std::size_t scope = 0;
            bool global = false;
        };
        struct LspDocumentIndex {
            std::string uri;
            std::string source;
            std::vector<Token> tokens;
            std::vector<std::size_t> tokenScope;
            std::vector<std::size_t> braceScope;
            std::vector<LspScopeInfo> scopes;
            std::vector<LspSymbolDecl> declarations;
        };
        std::map<std::string, LspDocumentIndex> workspaceSymbolIndexCache;
        const auto identifierAtPosition = [&](const std::string& source,
                std::size_t requestedLine, std::size_t requestedCharacter) {
            std::istringstream input(source);
            std::vector<std::string> sourceLines;
            std::string line;
            while (std::getline(input, line)) sourceLines.push_back(line);
            if (requestedLine >= sourceLines.size()) return std::string{};
            const std::string& selected = sourceLines[requestedLine];
            std::size_t cursor = byteOffsetForUtf16(selected, requestedCharacter);
            if (cursor > 0 &&
                (cursor == selected.size() ||
                 !identifierCharacter(static_cast<unsigned char>(selected[cursor]))) &&
                identifierCharacter(static_cast<unsigned char>(selected[cursor - 1]))) {
                --cursor;
            }
            try {
                const auto tokens = Lexer(source).tokenize();
                for (const auto& token : tokens) {
                    if (token.kind == TokenKind::String || token.kind == TokenKind::Integer ||
                        token.kind == TokenKind::Float || token.kind == TokenKind::End ||
                        token.pos.line != requestedLine + 1 || token.text.empty()) continue;
                    bool wordToken = true;
                    for (const unsigned char ch : token.text) {
                        if (!identifierCharacter(ch)) {
                            wordToken = false;
                            break;
                        }
                    }
                    if (!wordToken) continue;
                    const std::size_t start = token.pos.column > 0 ? token.pos.column - 1 : 0;
                    if (cursor >= start && cursor < start + token.text.size()) return token.text;
                }
            } catch (const std::exception&) {
                // An incomplete string/comment can make the current buffer un-tokenizable.
            }
            return std::string{};
        };
        const auto findIdentifierOccurrences = [&](const std::string& identifier,
                const std::string& requestUri, const std::string& requestSource,
                std::size_t requestedLine, std::size_t requestedCharacter) {
            std::vector<LspIdentifierOccurrence> found;
            if (identifier.empty()) return found;

            std::vector<LspDocumentIndex*> documents;
            for (const auto& document : collectWorkspaceDocuments(requestUri, requestSource)) {
                auto cachedIndex = workspaceSymbolIndexCache.find(document.first);
                if (cachedIndex != workspaceSymbolIndexCache.end() &&
                    cachedIndex->second.source == document.second) {
                    documents.push_back(&cachedIndex->second);
                    continue;
                }

                LspDocumentIndex indexed;
                indexed.uri = document.first;
                indexed.source = document.second;
                try {
                    indexed.tokens = Lexer(indexed.source).tokenize();
                } catch (const std::exception&) {
                    workspaceSymbolIndexCache.erase(document.first);
                    continue;
                }
                const std::size_t tokenCount = indexed.tokens.size();
                indexed.tokenScope.assign(tokenCount, 0);
                indexed.braceScope.assign(tokenCount, static_cast<std::size_t>(-1));
                indexed.scopes.push_back({static_cast<std::size_t>(-1), 0});
                std::vector<std::size_t> scopeStack{0};
                for (std::size_t i = 0; i < tokenCount; ++i) {
                    const TokenKind kind = indexed.tokens[i].kind;
                    if (kind == TokenKind::LBrace) {
                        indexed.tokenScope[i] = scopeStack.back();
                        const std::size_t parent = scopeStack.back();
                        const std::size_t scope = indexed.scopes.size();
                        indexed.scopes.push_back({parent, indexed.scopes[parent].depth + 1});
                        indexed.braceScope[i] = scope;
                        scopeStack.push_back(scope);
                    } else if (kind == TokenKind::RBrace) {
                        indexed.tokenScope[i] = scopeStack.back();
                        if (scopeStack.size() > 1) scopeStack.pop_back();
                    } else {
                        indexed.tokenScope[i] = scopeStack.back();
                    }
                }

                const auto addDeclaration = [&](std::size_t tokenIndex, std::size_t visibleFrom,
                                                std::size_t scope, const std::string& kind,
                                                bool global) {
                    if (tokenIndex >= tokenCount || indexed.tokens[tokenIndex].kind != TokenKind::Identifier) return;
                    const std::string name = indexed.tokens[tokenIndex].text;
                    const std::string identity = global
                        ? "global:" + kind + ":" + name
                        : "local:" + indexed.uri + ":" + std::to_string(tokenIndex);
                    indexed.declarations.push_back(
                        {name, identity, kind, tokenIndex, visibleFrom, scope, global});
                };

                for (std::size_t i = 1; i < tokenCount; ++i) {
                    if (indexed.tokenScope[i] != 0 || indexed.tokens[i].kind != TokenKind::Identifier) continue;
                    const TokenKind before = indexed.tokens[i - 1].kind;
                    if (before == TokenKind::KwFn) addDeclaration(i, 0, 0, "function", true);
                    else if (before == TokenKind::KwStruct) addDeclaration(i, 0, 0, "struct", true);
                    else if (before == TokenKind::KwEnum) addDeclaration(i, 0, 0, "enum", true);
                }

                for (std::size_t i = 0; i < tokenCount; ++i) {
                    const TokenKind kind = indexed.tokens[i].kind;
                    if (kind == TokenKind::KwLet || kind == TokenKind::KwConst) {
                        std::size_t nameIndex = i + 1;
                        if (kind == TokenKind::KwLet && nameIndex < tokenCount &&
                            indexed.tokens[nameIndex].kind == TokenKind::KwMut) ++nameIndex;
                        if (nameIndex < tokenCount &&
                            indexed.tokens[nameIndex].kind == TokenKind::Identifier) {
                            addDeclaration(nameIndex, nameIndex, indexed.tokenScope[nameIndex],
                                kind == TokenKind::KwConst ? "constant" : "variable", false);
                        }
                    } else if (kind == TokenKind::KwFor && i + 1 < tokenCount &&
                               indexed.tokens[i + 1].kind == TokenKind::Identifier) {
                        const std::size_t nameIndex = i + 1;
                        std::size_t bodyOpen = nameIndex + 1;
                        while (bodyOpen < tokenCount &&
                               indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                               indexed.tokens[bodyOpen].kind != TokenKind::Semicolon) ++bodyOpen;
                        if (bodyOpen < tokenCount && indexed.tokens[bodyOpen].kind == TokenKind::LBrace &&
                            indexed.braceScope[bodyOpen] != static_cast<std::size_t>(-1)) {
                            addDeclaration(nameIndex, bodyOpen + 1, indexed.braceScope[bodyOpen],
                                "variable", false);
                        }
                    } else if (kind == TokenKind::KwFn) {
                        std::size_t openParen = i + 1;
                        while (openParen < tokenCount &&
                               indexed.tokens[openParen].kind != TokenKind::LParen &&
                               indexed.tokens[openParen].kind != TokenKind::Semicolon) ++openParen;
                        if (openParen >= tokenCount || indexed.tokens[openParen].kind != TokenKind::LParen) continue;
                        std::size_t closeParen = openParen + 1;
                        std::size_t parenDepth = 1;
                        for (; closeParen < tokenCount && parenDepth > 0; ++closeParen) {
                            if (indexed.tokens[closeParen].kind == TokenKind::LParen) ++parenDepth;
                            else if (indexed.tokens[closeParen].kind == TokenKind::RParen) --parenDepth;
                        }
                        if (parenDepth != 0) continue;
                        --closeParen;
                        std::size_t bodyOpen = closeParen + 1;
                        while (bodyOpen < tokenCount &&
                               indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                               indexed.tokens[bodyOpen].kind != TokenKind::Semicolon) ++bodyOpen;
                        if (bodyOpen >= tokenCount || indexed.tokens[bodyOpen].kind != TokenKind::LBrace ||
                            indexed.braceScope[bodyOpen] == static_cast<std::size_t>(-1)) continue;
                        const std::size_t bodyScope = indexed.braceScope[bodyOpen];
                        for (std::size_t param = openParen + 1; param + 1 < closeParen; ++param) {
                            if (indexed.tokens[param].kind == TokenKind::Identifier &&
                                indexed.tokens[param + 1].kind == TokenKind::Colon) {
                                addDeclaration(param, bodyOpen + 1, bodyScope, "parameter", false);
                            }
                        }
                    }
                }
                auto indexedEntry = workspaceSymbolIndexCache.insert_or_assign(
                    indexed.uri, std::move(indexed)).first;
                documents.push_back(&indexedEntry->second);
            }

            const auto isAncestorScope = [](const LspDocumentIndex& document,
                                             std::size_t ancestor, std::size_t scope) {
                while (scope != static_cast<std::size_t>(-1)) {
                    if (scope == ancestor) return true;
                    if (scope >= document.scopes.size()) break;
                    scope = document.scopes[scope].parent;
                }
                return false;
            };
            const auto declarationAt = [](const LspDocumentIndex& document, std::size_t tokenIndex)
                -> const LspSymbolDecl* {
                for (const auto& declaration : document.declarations)
                    if (declaration.tokenIndex == tokenIndex) return &declaration;
                return nullptr;
            };
            const auto localIdentityAt = [&](const LspDocumentIndex& document, std::size_t tokenIndex) {
                if (tokenIndex >= document.tokens.size()) return std::string{};
                if (const auto* declaration = declarationAt(document, tokenIndex);
                    declaration && !declaration->global) return declaration->identity;
                if (document.tokens[tokenIndex].kind != TokenKind::Identifier) return std::string{};
                const std::string& name = document.tokens[tokenIndex].text;
                const LspSymbolDecl* selected = nullptr;
                for (const auto& declaration : document.declarations) {
                    if (declaration.global || declaration.name != name) continue;
                    const bool isOwnDeclaration = declaration.tokenIndex == tokenIndex;
                    const bool visible = tokenIndex >= declaration.visibleFrom &&
                        isAncestorScope(document, declaration.scope, document.tokenScope[tokenIndex]);
                    if (!isOwnDeclaration && !visible) continue;
                    if (!selected ||
                        document.scopes[declaration.scope].depth > document.scopes[selected->scope].depth ||
                        (document.scopes[declaration.scope].depth == document.scopes[selected->scope].depth &&
                         declaration.visibleFrom > selected->visibleFrom)) selected = &declaration;
                }
                return selected ? selected->identity : std::string{};
            };
            const auto isMemberOrFieldName = [&](const LspDocumentIndex& document, std::size_t tokenIndex) {
                if (tokenIndex >= document.tokens.size()) return true;
                const auto& tokens = document.tokens;
                if (tokenIndex > 0 &&
                    (tokens[tokenIndex - 1].kind == TokenKind::Dot ||
                     tokens[tokenIndex - 1].kind == TokenKind::ColonColon)) return true;
                if (tokenIndex + 1 < tokens.size() &&
                    tokens[tokenIndex + 1].kind == TokenKind::Colon &&
                    tokenIndex > 0 &&
                    (tokens[tokenIndex - 1].kind == TokenKind::LBrace ||
                     tokens[tokenIndex - 1].kind == TokenKind::Comma)) {
                    const auto* declaration = declarationAt(document, tokenIndex);
                    if (!declaration || declaration->kind != "parameter") return true;
                }
                return false;
            };
            const auto tokenAtPosition = [&](const LspDocumentIndex& document, std::size_t line,
                                             std::size_t character) {
                std::istringstream sourceStream(document.source);
                std::vector<std::string> sourceLines;
                std::string sourceLine;
                while (std::getline(sourceStream, sourceLine)) sourceLines.push_back(sourceLine);
                if (line >= sourceLines.size()) return document.tokens.size();
                std::size_t cursor = byteOffsetForUtf16(sourceLines[line], character);
                if (cursor > 0 && (cursor == sourceLines[line].size() ||
                    !identifierCharacter(static_cast<unsigned char>(sourceLines[line][cursor]))) &&
                    identifierCharacter(static_cast<unsigned char>(sourceLines[line][cursor - 1]))) --cursor;
                for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                    const Token& token = document.tokens[i];
                    if (token.kind != TokenKind::Identifier || token.pos.line != line + 1 ||
                        token.text != identifier || token.pos.column == 0) continue;
                    const std::size_t tokenStart = token.pos.column - 1;
                    if (cursor >= tokenStart && cursor < tokenStart + token.text.size()) return i;
                }
                return document.tokens.size();
            };

            LspDocumentIndex* requestedDocument = nullptr;
            for (auto* document : documents) {
                if (document->uri == requestUri) {
                    requestedDocument = document;
                    break;
                }
            }
            if (!requestedDocument) return found;
            const std::size_t selectedTokenIndex =
                tokenAtPosition(*requestedDocument, requestedLine, requestedCharacter);
            if (selectedTokenIndex >= requestedDocument->tokens.size()) return found;

            const LspSymbolDecl* selectedDeclaration = declarationAt(*requestedDocument, selectedTokenIndex);
            std::string targetIdentity;
            bool targetIsGlobal = false;
            std::string targetKind;
            if (selectedDeclaration) {
                targetIdentity = selectedDeclaration->identity;
                targetIsGlobal = selectedDeclaration->global;
                targetKind = selectedDeclaration->kind;
            } else {
                targetIdentity = localIdentityAt(*requestedDocument, selectedTokenIndex);
                if (targetIdentity.empty() &&
                    !isMemberOrFieldName(*requestedDocument, selectedTokenIndex)) {
                    const auto& tokens = requestedDocument->tokens;
                    std::string preferredKind;
                    if (selectedTokenIndex + 1 < tokens.size() &&
                        tokens[selectedTokenIndex + 1].kind == TokenKind::LParen) preferredKind = "function";
                    else if (selectedTokenIndex + 1 < tokens.size() &&
                             tokens[selectedTokenIndex + 1].kind == TokenKind::LBrace) preferredKind = "struct";
                    else if (selectedTokenIndex + 1 < tokens.size() &&
                             tokens[selectedTokenIndex + 1].kind == TokenKind::ColonColon) preferredKind = "enum";
                    const LspSymbolDecl* global = nullptr;
                    for (const auto* documentPointer : documents) {
                        const auto& document = *documentPointer;
                        for (const auto& declaration : document.declarations) {
                            if (!declaration.global || declaration.name != identifier) continue;
                            if (!preferredKind.empty() && declaration.kind != preferredKind) continue;
                            if (!global) global = &declaration;
                        }
                        if (global) break;
                    }
                    if (global) {
                        targetIdentity = global->identity;
                        targetIsGlobal = true;
                        targetKind = global->kind;
                    }
                }
            }
            if (targetIdentity.empty()) return found;

            for (const auto* documentPointer : documents) {
                const auto& document = *documentPointer;
                for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                    const Token& token = document.tokens[i];
                    if (token.kind != TokenKind::Identifier || token.text != identifier) continue;
                    const LspSymbolDecl* declaration = declarationAt(document, i);
                    bool matches = false;
                    if (!targetIsGlobal) {
                        matches = document.uri == requestedDocument->uri &&
                            localIdentityAt(document, i) == targetIdentity;
                    } else {
                        if (declaration && declaration->global) {
                            matches = declaration->identity == targetIdentity;
                        } else if (!isMemberOrFieldName(document, i) &&
                                   localIdentityAt(document, i).empty()) {
                            matches = true;
                        }
                    }
                    if (!matches) continue;
                    if (document.uri == requestedDocument->uri && i == selectedTokenIndex) {
                        // Keep the cursor token even for unusual valid syntax.
                    }
                    std::istringstream sourceStream(document.source);
                    std::string sourceLine;
                    for (std::size_t currentLine = 1;
                         currentLine < token.pos.line && std::getline(sourceStream, sourceLine);
                         ++currentLine) {}
                    if (token.pos.line == 0) continue;
                    const std::size_t lineIndex = token.pos.line - 1;
                    const std::size_t startByte = token.pos.column > 0 ? token.pos.column - 1 : 0;
                    sourceLine.clear();
                    std::istringstream lineStream(document.source);
                    for (std::size_t currentLine = 0; currentLine <= lineIndex && std::getline(lineStream, sourceLine); ++currentLine) {
                        if (currentLine == lineIndex) break;
                    }
                    const std::size_t startCharacter = utf16Length(sourceLine.substr(0, std::min(startByte, sourceLine.size())));
                    const LspSymbolDecl* occurrenceDeclaration = declarationAt(document, i);
                    const bool isDeclaration = occurrenceDeclaration &&
                        occurrenceDeclaration->identity == targetIdentity;
                    found.push_back({document.uri, lineIndex, startCharacter,
                                     startCharacter + utf16Length(token.text), isDeclaration});
                }
            }
            return found;
        };
        const auto isValidIdentifier = [&](const std::string& candidate) {
            try {
                const auto tokens = Lexer(candidate).tokenize();
                return tokens.size() == 2 &&
                    tokens[0].kind == TokenKind::Identifier &&
                    tokens[0].text == candidate &&
                    tokens[1].kind == TokenKind::End;
            } catch (const std::exception&) {
                return false;
            }
        };
        const auto publishDiagnostics = [&](const std::string& uri, const std::string& source) {
            const auto diagnostics = collectDiagnostics(source);
            std::ostringstream payload;
            payload << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
                    << "\"params\":{\"uri\":\"" << jsonEscape(uri) << "\",\"diagnostics\":[";
            for (std::size_t i = 0; i < diagnostics.size(); ++i) {
                if (i) payload << ',';
                const auto& diagnostic = diagnostics[i];
                payload << "{\"range\":{\"start\":{\"line\":" << diagnostic.line
                        << ",\"character\":" << diagnostic.character
                        << "},\"end\":{\"line\":" << diagnostic.line
                        << ",\"character\":" << diagnostic.character + 1
                        << "}},\"severity\":1,\"code\":\"" << diagnostic.code
                        << "\",\"source\":\"nexus\",\"message\":\""
                        << jsonEscape(diagnostic.message) << "\"}";
            }
            payload << "]}}";
            send(payload.str());
        };
        const std::vector<std::pair<std::string, std::string>> completions = {
            {"fn", "Function declaration"},
            {"let", "Variable declaration"},
            {"const", "Constant declaration"},
            {"mut", "Mutable binding modifier"},
            {"if", "Conditional"},
            {"else", "Alternative branch"},
            {"while", "Loop"},
            {"for", "Range or collection loop"},
            {"match", "Pattern matching"},
            {"return", "Return from function"},
            {"struct", "Struct declaration"},
            {"enum", "Enum declaration"},
            {"import std.io", "Import standard input/output"},
            {"import std.fs", "Import standard file system"},
            {"import std.net", "Import standard networking"},
            {"import std.json", "Import standard JSON helpers"},
            {"print", "Write a value to stdout"},
            {"input", "Read a line from stdin"},
            {"input_i64", "Read an integer from stdin"},
            {"str_i64", "Convert an integer to a string"},
            {"file_read", "Read a file"},
            {"file_write", "Write a file"},
            {"file_exists", "Check whether a file exists"},
            {"env", "Read an environment variable"},
            {"sleep", "Pause execution"},
            {"gfx_window", "Create a graphics window"},
            {"gfx_clear", "Clear the graphics frame"},
            {"gfx_present", "Present the graphics frame"}
        };
        std::string header;
        while (std::cin) {
            std::size_t contentLength = 0;
            bool sawHeader = false;
            while (std::getline(std::cin, header)) {
                if (!header.empty() && header.back() == '\r') header.pop_back();
                if (header.empty()) break;
                if (header.rfind("Content-Length:", 0) == 0) {
                    const std::string value = trim(header.substr(15));
                    contentLength = static_cast<std::size_t>(std::stoull(value));
                    sawHeader = true;
                }
            }
            if (!sawHeader) break;
            if (contentLength > 16 * 1024 * 1024) {
                std::cerr << "NEXUS LSP: message exceeds 16 MiB limit\n";
                return 2;
            }
            std::string body(contentLength, '\0');
            std::cin.read(body.data(), static_cast<std::streamsize>(contentLength));
            if (static_cast<std::size_t>(std::cin.gcount()) != contentLength) {
                std::cerr << "NEXUS LSP: incomplete JSON-RPC message\n";
                return 2;
            }

            const std::string method = extractString(body, "method");
            const std::string id = extractId(body);
            if (method == "exit") break;
            if (method == "initialize" && !id.empty()) {
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                    ",\"result\":{\"serverInfo\":{\"name\":\"nexus-lsp\",\"version\":\"" +
                    kVersion + "\"},\"capabilities\":{\"textDocumentSync\":1,"
                    "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":[\".\",\":\"]},"
                    "\"definitionProvider\":true,\"referencesProvider\":true,\"renameProvider\":true,\"hoverProvider\":true,\"documentFormattingProvider\":true}}}");
            } else if (method == "shutdown" && !id.empty()) {
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":null}");
            } else if (method == "textDocument/didOpen") {
                const std::string uri = extractString(body, "uri");
                if (!uri.empty()) {
                    const std::string source = extractString(body, "text");
                    openDocuments[uri] = source;
                    publishDiagnostics(uri, source);
                }
            } else if (method == "textDocument/didChange") {
                const std::string uri = extractString(body, "uri");
                if (!uri.empty()) {
                    // The server advertises full-document synchronization.
                    const std::string source = extractString(body, "text");
                    openDocuments[uri] = source;
                    publishDiagnostics(uri, source);
                }
            } else if (method == "textDocument/didSave") {
                const std::string uri = extractString(body, "uri");
                workspaceDiskCache.clear();
                const auto document = openDocuments.find(uri);
                if (document != openDocuments.end()) publishDiagnostics(uri, document->second);
            } else if (method == "workspace/didChangeWatchedFiles") {
                // Rebuild disk symbols after create/delete/change notifications.
                workspaceDiskCache.clear();
            } else if (method == "textDocument/didClose") {
                const std::string uri = extractString(body, "uri");
                if (!uri.empty()) {
                    openDocuments.erase(uri);
                    workspaceDiskCache.clear();
                    send("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
                         "\"params\":{\"uri\":\"" + jsonEscape(uri) + "\",\"diagnostics\":[]}}");
                }

            } else if (method == "textDocument/hover" && !id.empty()) {
                const std::string uri = extractString(body, "uri");
                const auto document = openDocuments.find(uri);
                std::string result = "null";
                if (document != openDocuments.end()) {
                    const std::string identifier = identifierAtPosition(document->second,
                        extractNumber(body, "line"), extractNumber(body, "character"));
                    for (const auto& item : completions) {
                        if (item.first == identifier) {
                            const std::string markdown = "**" + item.first + "**\n\n" + item.second;
                            result = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                jsonEscape(markdown) + "\"}}";
                            break;
                        }
                    }

                    // When the cursor is on a user-defined declaration, parse the
                    // current open buffer and show its real signature. Incomplete
                    // editor buffers can fail to parse; hover should then fall back
                    // to null instead of disrupting the LSP session.
                    if (result == "null" && !identifier.empty()) {
                        try {
                            Program program = Parser(Lexer(document->second).tokenize()).parseProgram();
                            const auto describeType = [&](const auto& self, const Type& type) -> std::string {
                                switch (type.kind) {
                                    case TypeKind::I64: return "i64";
                                    case TypeKind::F64: return "f64";
                                    case TypeKind::Bool: return "bool";
                                    case TypeKind::String: return "string";
                                    case TypeKind::Void: return "void";
                                    case TypeKind::Pointer:
                                        return type.pointee ? "*" + self(self, *type.pointee) : "*unknown";
                                    case TypeKind::Struct:
                                        return type.name.empty() ? "struct" : type.name;
                                    case TypeKind::Enum:
                                        return type.name.empty() ? "enum" : type.name;
                                    case TypeKind::Array:
                                        return type.element
                                            ? "[" + self(self, *type.element) + "; " + std::to_string(type.arraySize) + "]"
                                            : "[unknown]";
                                    case TypeKind::Unknown: return "inferred";
                                }
                                return "unknown";
                            };
                            std::string markdown;
                            for (const auto& function : program.functions) {
                                if (function.name != identifier) continue;
                                std::ostringstream signature;
                                signature << "fn " << function.name << "(";
                                for (std::size_t i = 0; i < function.params.size(); ++i) {
                                    if (i) signature << ", ";
                                    signature << function.params[i].name << ": "
                                              << describeType(describeType, function.params[i].type);
                                }
                                signature << ") -> " << describeType(describeType, function.ret);
                                markdown = "**" + signature.str() + "**\n\nUser-defined function.";
                                break;
                            }
                            if (markdown.empty()) {
                                for (const auto& declaration : program.structs) {
                                    if (declaration.name != identifier) continue;
                                    std::ostringstream description;
                                    description << "**struct " << declaration.name << "**";
                                    if (declaration.fields.empty()) {
                                        description << "\n\nNo fields.";
                                    } else {
                                        description << "\n\nFields:";
                                        for (const auto& field : declaration.fields) {
                                            description << "\n- `" << field.name << "`: "
                                                << describeType(describeType, field.type) << "`";
                                        }
                                    }
                                    markdown = description.str();
                                    break;
                                }
                            }
                            if (markdown.empty()) {
                                for (const auto& declaration : program.enums) {
                                    if (declaration.name != identifier) continue;
                                    std::ostringstream description;
                                    description << "**enum " << declaration.name << "**";
                                    if (declaration.variants.empty()) {
                                        description << "\n\nNo variants.";
                                    } else {
                                        description << "\n\nVariants:";
                                        for (const auto& variant : declaration.variants) {
                                            description << "\n- `" << variant << "`";
                                        }
                                    }
                                    markdown = description.str();
                                    break;
                                }
                            }
                            if (!markdown.empty()) {
                                result = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                    jsonEscape(markdown) + "\"}}";
                            }
                        } catch (const std::exception&) {
                            // Unsaved/incomplete source is normal while an editor is open.
                        }
                    }
                }
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
            } else if ((method == "textDocument/completion" || method == "completion") && !id.empty()) {
                std::ostringstream result;
                result << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":[";
                for (std::size_t i = 0; i < completions.size(); ++i) {
                    if (i) result << ',';
                    std::string escaped;
                    for (const char c : completions[i].first) {
                        if (c == '"' || c == '\\') escaped.push_back('\\');
                        escaped.push_back(c);
                    }
                    result << "{\"label\":\"" << escaped
                           << "\",\"kind\":14,\"detail\":\"" << completions[i].second << "\"}";
                }
                result << "]}";
                send(result.str());
            } else if (method == "textDocument/formatting" && !id.empty()) {
                const std::string uri = extractString(body, "uri");
                const auto doc = openDocuments.find(uri);
                std::string result = "[]";
                if (doc != openDocuments.end()) {
                    const std::string formatted = formatNexusSource(doc->second);
                    if (formatted != doc->second) {
                        const std::size_t endLine = static_cast<std::size_t>(
                            std::count(doc->second.begin(), doc->second.end(), '\n'));
                        std::size_t endCharacter = 0;
                        if (!doc->second.empty() && doc->second.back() != '\n') {
                            const auto lastNewline = doc->second.find_last_of('\n');
                            const std::size_t lineStart =
                                lastNewline == std::string::npos ? 0 : lastNewline + 1;
                            endCharacter = utf16Length(doc->second.substr(lineStart));
                        }
                        result = "[{\"range\":{\"start\":{\"line\":0,\"character\":0},"
                            "\"end\":{\"line\":" + std::to_string(endLine) +
                            ",\"character\":" + std::to_string(endCharacter) +
                            "}},\"newText\":\"" + jsonEscape(formatted) + "\"}]";
                    }
                }
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");

            } else if (method == "textDocument/references" && !id.empty()) {
                const std::string uri = extractString(body, "uri");
                const auto document = openDocuments.find(uri);
                std::string result = "[]";
                if (document != openDocuments.end()) {
                    const std::string identifier = identifierAtPosition(document->second,
                        extractNumber(body, "line"), extractNumber(body, "character"));
                    const auto occurrences = findIdentifierOccurrences(identifier, uri, document->second,
                        extractNumber(body, "line"), extractNumber(body, "character"));
                    std::ostringstream payload;
                    payload << "[";
                    for (std::size_t i = 0; i < occurrences.size(); ++i) {
                        if (i) payload << ',';
                        const auto& occurrence = occurrences[i];
                        payload << "{\"uri\":\"" << jsonEscape(occurrence.uri)
                            << "\",\"range\":{\"start\":{\"line\":"
                            << occurrence.line << ",\"character\":" << occurrence.startCharacter
                            << "},\"end\":{\"line\":" << occurrence.line
                            << ",\"character\":" << occurrence.endCharacter << "}}}";
                    }
                    payload << "]";
                    result = payload.str();
                }
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
            } else if (method == "textDocument/rename" && !id.empty()) {
                const std::string uri = extractString(body, "uri");
                const auto document = openDocuments.find(uri);
                const std::string newName = extractString(body, "newName");
                if (!isValidIdentifier(newName)) {
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"error\":{\"code\":-32602,\"message\":\"newName must be a valid NEXUS identifier\"}}");
                } else {
                    std::vector<LspIdentifierOccurrence> occurrences;
                    if (document != openDocuments.end()) {
                        const std::string identifier = identifierAtPosition(document->second,
                            extractNumber(body, "line"), extractNumber(body, "character"));
                        occurrences = findIdentifierOccurrences(identifier, uri, document->second,
                            extractNumber(body, "line"), extractNumber(body, "character"));
                    }
                    std::set<std::string> uris;
                    for (const auto& occurrence : occurrences) uris.insert(occurrence.uri);
                    std::ostringstream changes;
                    changes << "{\"changes\":{";
                    bool firstUri = true;
                    for (const auto& changedUri : uris) {
                        if (!firstUri) changes << ',';
                        firstUri = false;
                        changes << "\"" << jsonEscape(changedUri) << "\":[";
                        bool firstEdit = true;
                        for (const auto& occurrence : occurrences) {
                            if (occurrence.uri != changedUri) continue;
                            if (!firstEdit) changes << ',';
                            firstEdit = false;
                            changes << "{\"range\":{\"start\":{\"line\":" << occurrence.line
                                << ",\"character\":" << occurrence.startCharacter
                                << "},\"end\":{\"line\":" << occurrence.line
                                << ",\"character\":" << occurrence.endCharacter
                                << "}},\"newText\":\"" << jsonEscape(newName) << "\"}";
                        }
                        changes << "]";
                    }
                    changes << "}}";
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"result\":" + changes.str() + "}");
                }
            } else if (method == "textDocument/definition" && !id.empty()) {
                const std::string uri = extractString(body, "uri");
                const auto doc = openDocuments.find(uri);
                std::string result = "null";
                const std::size_t requestedLine = extractNumber(body, "line");
                const std::size_t requestedCharacter = extractNumber(body, "character");
                if (doc != openDocuments.end()) {
                    const std::string identifier = identifierAtPosition(
                        doc->second, requestedLine, requestedCharacter);
                    const auto occurrences = findIdentifierOccurrences(
                        identifier, uri, doc->second, requestedLine, requestedCharacter);
                    for (const auto& occurrence : occurrences) {
                        if (!occurrence.isDeclaration) continue;
                        std::ostringstream location;
                        location << "{\"uri\":\"" << jsonEscape(occurrence.uri)
                                 << "\",\"range\":{\"start\":{\"line\":" << occurrence.line
                                 << ",\"character\":" << occurrence.startCharacter
                                 << "},\"end\":{\"line\":" << occurrence.line
                                 << ",\"character\":" << occurrence.endCharacter << "}}}";
                        result = location.str();
                        break;
                    }
                }
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
            } else if (!method.empty() && !id.empty()) {
                send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                     ",\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
            }
        }
        return 0;
    }

} // namespace

int run(int argc, char** argv) {

    try {

        gExecutable = argc > 0 ? fs::absolute(argv[0]) : fs::path();

        if (argc < 2) {
            usage();
            return 1;
        }
        const std::string firstArg = argv[1];
        std::string firstExtension = fs::path(firstArg).extension().string();
        std::transform(firstExtension.begin(), firstExtension.end(), firstExtension.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        const bool directFileInvocation = (firstExtension == ".nx");
        const std::string cmd = directFileInvocation ? "run" : firstArg;

        if (cmd == "help" || cmd == "--help") {
            usage();
            return 0;
        }
        if (cmd == "version" || cmd == "--version") {

            std::cout << "Nexus " << kVersion << " (C++20/LLVM IR + Native Graphics/Audio/Game Runtime)\n";

            return 0;

        }
        if (cmd == "targets") {

            std::cout << "native\nx86_64-linux\naarch64-linux\nx86_64-windows\naarch64-windows\n"
            "x86_64-macos\naarch64-macos\naarch64-android\nwasm32-wasi\n"
            "\nbuiltin native backend (no external toolchain):\n"
            "  x86_64-windows: PE32+ console executable\n"
            "  x86_64-linux:   ELF64 executable\n"
            "  native:         host default (x86-64 hosts only)\n";

            return 0;

        }

        if (cmd == "new") {

            if (argc < 3) throw std::runtime_error("new requires a project name");

            std::string templ = "cli";

            for (int i = 3; i < argc; ++i) {

                const std::string a = argv[i];

                if (a == "--template" && i + 1 < argc) templ = argv[++i];

                else if (a.rfind("--template=", 0) == 0) templ = a.substr(11);

                else throw std::runtime_error("unknown option: " + a);

            }
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" && templ != "paint" && templ != "server" && templ != "library") throw std::runtime_error("unknown template: " + templ);

            const fs::path projectPath = fs::absolute(argv[2]);

            return initProject(projectPath, projectPath.filename().string(), templ, true);

        }

        if (cmd == "init") {

            std::string templ = "cli";

            for (int i = 2; i < argc; ++i) {

                const std::string a = argv[i];

                if (a == "--template" && i + 1 < argc) templ = argv[++i];

                else if (a.rfind("--template=", 0) == 0) templ = a.substr(11);

                else throw std::runtime_error("unknown option: " + a);

            }
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" && templ != "paint" && templ != "server" && templ != "library") throw std::runtime_error("unknown template: " + templ);

            const fs::path root = fs::current_path();

            return initProject(root, root.filename().string(), templ, false);

        }

        if (cmd == "info") return commandInfo();

        if (cmd == "assets") return commandAssets();

        if (cmd == "clean") return commandClean();

        if (cmd == "doctor") {
            bool json = false;
            for (int i = 2; i < argc; ++i) {
                const std::string option = argv[i];
                if (option == "--json") json = true;
                else throw std::runtime_error("unknown doctor option: " + option);
            }
            return commandDoctor(json);
        }

        if (cmd == "self-test") return commandSelfTest();

        if (cmd == "explain-backend") {
            if (argc < 3) throw std::runtime_error(
                "explain-backend requires a source file (usage: nexus explain-backend file.nx [--target TRIPLE])");
            std::string target = "native";
            for (int i = 3; i < argc; ++i) {
                const std::string option = argv[i];
                if (option == "--target" && i + 1 < argc) target = argv[++i];
                else if (option.rfind("--target=", 0) == 0) target = option.substr(9);
                else throw std::runtime_error("unknown explain-backend option: " + option);
            }
            return commandExplainBackend(resolveInput(argv[2]), target);
        }

        if (cmd == "fmt" || cmd == "format") {
            bool checkOnly = false;
            std::string input;
            for (int i = 2; i < argc; ++i) {
                const std::string option = argv[i];
                if (option == "--check") checkOnly = true;
                else if (!option.empty() && option.front() == '-')
                    throw std::runtime_error("unknown fmt option: " + option);
                else if (input.empty()) input = option;
                else throw std::runtime_error("fmt accepts only one source file");
            }
            return commandFmt(resolveInput(input), checkOnly);
        }
        if (cmd == "lint") {

            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

            return commandLint(in);

        }
        if (cmd == "doc") return commandDoc();

        if (cmd == "bench") {

            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

            int runs=5;
            for(int i=2;i<argc;++i){
                std::string a=argv[i];
                if(a=="--runs"&&i+1<argc)runs=std::stoi(argv[++i]);
                else if(a.rfind("--runs=",0)==0)runs=std::stoi(a.substr(7));
            }
            return commandBench(in,runs);

        }
        if (cmd == "test") return commandTest();

        if (cmd == "export") {
            std::vector<std::string> args;
            for(int i=2;i<argc;++i) args.emplace_back(argv[i]);
            return commandExport(args);
        }

        if (cmd == "editor") {

            bool gui=false;
            for(int i=2;i<argc;++i) if(std::string(argv[i])=="--gui") gui=true;

            if(!gui) return commandEditor();

            const auto tpl=templateSource("editor");
            if(tpl.empty()) throw std::runtime_error("built-in editor template not found");

            const auto out=fs::temp_directory_path()/"nexus-editor";
            compileFile(tpl,out,"native",false,2);

            return runCommand({out.string()});

        }
        if (cmd == "lsp") return commandLsp();

        if (cmd == "pkg") {
            std::vector<std::string> args;
            for(int i=2;i<argc;++i) args.emplace_back(argv[i]);
            return commandPkg(args);
        }

        if (cmd == "check") {

            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

            std::set<fs::path> seen;

            auto prog = loadProgram(in, seen);

            SemanticAnalyzer().analyze(prog);

            std::cout << "check: OK\n";

            return 0;

        }

        if (cmd == "build" || cmd == "run") {

            int argi = directFileInvocation ? 1 : 2;

            std::string input;

            if (argi < argc && argv[argi][0] != '-') input = argv[argi++];

            ProjectConfig cfg;

            bool hasProject = false;

            try {
                cfg = loadProject();
                hasProject = true;
            } catch (...) {
            }
            const auto in = resolveInput(input);

            std::string target = hasProject && input.empty() ? cfg.target : "native";

            std::string explicitOutput;

            bool emitIR = false;

            bool release = false;

            std::string backend = "auto";

            for (int i = argi; i < argc; ++i) {

                const std::string a = argv[i];

                if (a == "--emit-ir") emitIR = true;

                else if (a == "--release") release = true;

                else if (a == "-o" && i + 1 < argc) explicitOutput = argv[++i];

                else if (a.rfind("--target=", 0) == 0) target = a.substr(9);

                else if (a == "--target" && i + 1 < argc) target = argv[++i];

                else if (a.rfind("--backend=", 0) == 0) backend = a.substr(10);

                else if (a == "--backend" && i + 1 < argc) backend = argv[++i];

                else throw std::runtime_error("unknown option: " + a);

            }
            const std::string resolvedTriple = targetTriple(target);
            bool windowsTarget = false;
            #ifdef _WIN32
            windowsTarget = target.empty() || target == "native" ||
                resolvedTriple.find("windows") != std::string::npos;
            #else
            windowsTarget = resolvedTriple.find("windows") != std::string::npos;
            #endif

            const auto out = defaultOutput(in, explicitOutput, release, windowsTarget);

            const int rc = compileFile(in, out, target, emitIR, release ? 3 : 2, backend);

            if (cmd == "run" && rc == 0) {
                const fs::path root = hasProject && input.empty() ? cfg.root : out.parent_path();
                return launchExecutable(out, root);
            }
            std::cout << "build: OK -> " << out << "\n";

            return rc;

        }

        usage();

        return 1;

    } catch (const std::exception& e) {

        std::cerr << formatDiagnostic(e) << "\n";

        return 2;

    }
}

} // namespace nexus
