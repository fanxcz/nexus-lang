#include "driver/driver.hpp"

#include "codegen/llvm_ir.hpp"
#include "format/formatter.hpp"
#include "lexer/lexer.hpp"
#include "native/native.hpp"
#include "package/sha256.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
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

            if (!f)
                throw std::runtime_error("cannot open " + p.string());

            return {std::istreambuf_iterator<char>(f), {}};
        }

        std::string trim(std::string s) {

            const auto first = s.find_first_not_of(" \t\r\n");

            if (first == std::string::npos)
                return {};

            const auto last = s.find_last_not_of(" \t\r\n");

            return s.substr(first, last - first + 1);
        }

        std::string formatDiagnostic(const std::exception& error,
                                     const std::string& code = "NX1001") {
            const std::string raw = error.what();
            if (raw.rfind("NEXUS found ", 0) == 0 &&
                (raw.find(" syntax error") != std::string::npos ||
                 raw.find(" semantic error") != std::string::npos)) {
                const std::string nestedCode =
                    raw.find(" semantic error") != std::string::npos ? "NX2001" : "NX1001";
                std::istringstream input(raw);
                std::string line;
                std::getline(input, line); // The first line is the summary count.
                std::ostringstream combined;
                bool first = true;
                while (std::getline(input, line)) {
                    if (line.rfind("  - ", 0) != 0)
                        continue;
                    const std::runtime_error one(line.substr(4));
                    if (!first)
                        combined << '\n';
                    combined << formatDiagnostic(one, nestedCode);
                    first = false;
                }
                if (!first)
                    return combined.str();
            }
            const auto marker = raw.rfind(" at ");
            if (marker != std::string::npos) {
                const std::string_view location(raw.data() + marker + 4, raw.size() - marker - 4);
                const auto separator = location.find(':');
                if (separator != std::string_view::npos &&
                    location.find(':', separator + 1) == std::string_view::npos) {
                    const auto parseNumber = [](std::string_view value, std::size_t& result) {
                        if (value.empty())
                            return false;
                        result = 0;
                        for (const char c : value) {
                            if (c < '0' || c > '9')
                                return false;
                            result = result * 10 + static_cast<std::size_t>(c - '0');
                        }
                        return result > 0;
                    };
                    std::size_t line = 0;
                    std::size_t column = 0;
                    std::size_t columnEnd = separator + 1;
                    while (columnEnd < location.size() &&
                           location[columnEnd] >= '0' && location[columnEnd] <= '9') {
                        ++columnEnd;
                    }
                    if (parseNumber(location.substr(0, separator), line) &&
                        parseNumber(location.substr(separator + 1,
                                                    columnEnd - separator - 1),
                                    column) &&
                        !gDiagnosticSource.empty()) {
                        std::ostringstream out;
                        out << gDiagnosticSource.string() << ":" << line << ":" << column
                            << ": error[" << code << "]: " << trim(raw.substr(0, marker));
                        std::istringstream source(gDiagnosticText);
                        std::string sourceLine;
                        for (std::size_t current = 1;
                             current <= line && std::getline(source, sourceLine);
                             ++current) {
                            if (current == line) {
                                const std::string label = "  " + std::to_string(line) + " | ";
                                out << "\n"
                                    << label << sourceLine << "\n"
                                    << std::string(label.size(), ' ')
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

        std::string
        tomlString(const std::string& data, const std::string& section, const std::string& key) {

            std::istringstream in(data);

            std::string line;

            std::string current;

            while (std::getline(in, line)) {

                line = trim(line);

                if (line.empty() || line[0] == '#')
                    continue;

                if (line.front() == '[' && line.back() == ']') {

                    current = trim(line.substr(1, line.size() - 2));

                    continue;
                }
                if (current != section)
                    continue;

                const auto eq = line.find('=');

                if (eq == std::string::npos)
                    continue;

                const auto lhs = trim(line.substr(0, eq));

                if (lhs != key)
                    continue;

                const auto rhs = trim(line.substr(eq + 1));

                if (rhs.size() >= 2 && rhs.front() == '"' && rhs.back() == '"')
                    return rhs.substr(1, rhs.size() - 2);

                return rhs;
            }
            return {};
        }

        int64_t tomlInt(const std::string& data,
                        const std::string& section,
                        const std::string& key,
                        int64_t fallback) {

            const std::string value = tomlString(data, section, key);

            if (value.empty())
                return fallback;

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

            if (ec)
                start = fs::absolute(start, ec);

            if (fs::is_regular_file(start, ec))
                start = start.parent_path();

            for (;;) {

                if (fs::exists(start / "nexus.toml"))
                    return start;

                const auto parent = start.parent_path();

                if (parent == start)
                    break;

                start = parent;
            }
            return {};
        }

        ProjectConfig loadProject(const fs::path& requestedRoot = fs::current_path()) {

            const fs::path root = findProjectRoot(requestedRoot);

            if (root.empty())
                throw std::runtime_error(
                    "nexus.toml not found; run `nexus init` or pass a .nx file");

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

            if (cfg.name.empty())
                cfg.name = root.filename().string();

            if (cfg.version.empty())
                cfg.version = kVersion;

            if (cfg.entry.empty())
                cfg.entry = "src/main.nx";

            if (cfg.artifact.empty())
                cfg.artifact = cfg.name;

            if (cfg.windowTitle.empty())
                cfg.windowTitle = cfg.name;

            return cfg;
        }

        // Launch child processes directly instead of routing through a shell.
        // The previous implementation built a single command string and passed
        // it to std::system(), which breaks for quoted program paths containing
        // spaces (cmd.exe strips the outer quotes) and needlessly depends on a
        // command interpreter. runCommand() uses CreateProcessW on Windows and
        // fork()/execvp() on POSIX, so no shell quoting is required at all.
        // Returns the child exit code, or 127 when the program cannot start.
        int runCommand(const std::vector<std::string>& argv,
                       const fs::path& workDir = fs::path(),
                       bool quiet = false) {
            if (argv.empty())
                throw std::runtime_error("cannot run an empty command");
#ifdef _WIN32
            const auto widen = [](const std::string& s) {
                if (s.empty())
                    return std::wstring();
                const int n = MultiByteToWideChar(
                    CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
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
                if (!need)
                    return w;
                std::wstring out;
                out.push_back(L'"');
                std::size_t backslashes = 0;
                for (const wchar_t c : w) {
                    if (c == L'\\') {
                        ++backslashes;
                        out.push_back(c);
                    } else if (c == L'"') {
                        for (std::size_t i = 0; i < backslashes; ++i)
                            out.push_back(L'\\');
                        backslashes = 0;
                        out.push_back(L'\\');
                        out.push_back(L'"');
                    } else {
                        backslashes = 0;
                        out.push_back(c);
                    }
                }
                for (std::size_t i = 0; i < backslashes; ++i)
                    out.push_back(L'\\');
                out.push_back(L'"');
                return out;
            };

            std::vector<std::string> full = argv;
            std::wstring launcher;
            const std::string ext = fs::path(argv[0]).extension().string();
            const bool isBatch = ext == ".cmd" || ext == ".CMD" || ext == ".bat" || ext == ".BAT";
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
                for (const auto& a : argv)
                    full.push_back(a);
            }
            std::wstring commandLine;
            for (std::size_t i = 0; i < full.size(); ++i) {
                if (i)
                    commandLine.push_back(L' ');
                commandLine += quoteArg(full[i]);
            }
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            HANDLE nul = INVALID_HANDLE_VALUE;
            if (quiet) {
                nul = CreateFileW(L"NUL",
                                  GENERIC_WRITE,
                                  FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
                if (nul != INVALID_HANDLE_VALUE) {
                    startup.hStdOutput = nul;
                    startup.hStdError = nul;
                    startup.dwFlags |= STARTF_USESTDHANDLES;
                }
            }
            PROCESS_INFORMATION process{};
            const std::wstring workDirW =
                workDir.empty() ? std::wstring() : widen(fs::absolute(workDir).string());
            const BOOL started = CreateProcessW(nullptr,
                                                commandLine.data(),
                                                nullptr,
                                                nullptr,
                                                FALSE,
                                                0,
                                                nullptr,
                                                workDir.empty() ? nullptr : workDirW.c_str(),
                                                &startup,
                                                &process);
            const DWORD spawnError = GetLastError();
            if (nul != INVALID_HANDLE_VALUE)
                CloseHandle(nul);
            if (!started) {
                std::cerr << "nexus: cannot start '" << argv[0] << "' (Windows error " << spawnError
                          << ")\n";
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
                std::cerr << "nexus: cannot fork for '" << argv[0] << "': " << std::strerror(errno)
                          << "\n";
                return 127;
            }
            if (pid == 0) {
                if (!workDir.empty() && chdir(workDir.c_str()) != 0)
                    _exit(127);
                if (quiet) {
                    const int fd = open("/dev/null", O_WRONLY);
                    if (fd >= 0) {
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        if (fd > STDERR_FILENO)
                            close(fd);
                    }
                }
                std::vector<char*> args;
                args.reserve(argv.size() + 1);
                for (const auto& a : argv)
                    args.push_back(const_cast<char*>(a.c_str()));
                args.push_back(nullptr);
                execvp(args[0], args.data());
                _exit(127);
            }
            int status = 0;
            while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) {
                    std::cerr << "nexus: failed while waiting for '" << argv[0]
                              << "': " << std::strerror(errno) << "\n";
                    return 127;
                }
            }
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            return 1;
#endif
        }

        void usage() {

            std::cout
                << "Nexus " << kVersion << "\n"
                << "Usage: nexus <command> [options]\n\n"
                << "Interactive shell:\n"
                << "  repl                 evaluate expressions (also starts by running nexus with "
                   "no arguments)\n\n"
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
                << "  build [file.nx] [-o output] [--target TRIPLE] [--backend auto|native|llvm] "
                   "[--emit-ir] [--release]\n"
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

        void appendProgram(Program& dst, Program src) {

            dst.imports.insert(dst.imports.end(), src.imports.begin(), src.imports.end());

            for (auto& s : src.structs)
                dst.structs.push_back(std::move(s));

            for (auto& e : src.enums)
                dst.enums.push_back(std::move(e));

            for (auto& f : src.functions)
                dst.functions.push_back(std::move(f));
        }

        struct ModuleLoadContext {
            std::set<fs::path> active;
            std::set<fs::path> loaded;
            std::vector<fs::path> stack;
        };

        Program loadProgram(const fs::path& root, ModuleLoadContext& context) {

            const auto canon = fs::weakly_canonical(root);

            if (context.active.contains(canon)) {
                std::ostringstream cycle;
                cycle << "circular import detected: ";
                const auto begin = std::find(context.stack.begin(), context.stack.end(), canon);
                for (auto it = begin; it != context.stack.end(); ++it) {
                    if (it != begin)
                        cycle << " -> ";
                    cycle << it->generic_string();
                }
                if (begin != context.stack.end())
                    cycle << " -> ";
                cycle << canon.generic_string();
                throw std::runtime_error(cycle.str());
            }

            if (context.loaded.contains(canon))
                return {};

            context.active.insert(canon);
            context.stack.push_back(canon);
            struct ModuleLoadGuard {
                ModuleLoadContext& context;
                fs::path path;
                ~ModuleLoadGuard() {
                    context.active.erase(path);
                    if (!context.stack.empty())
                        context.stack.pop_back();
                }
            } guard{context, canon};

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
                    for (const auto& c : candidates)
                        if (fs::exists(c)) {
                            child = c;
                            break;
                        }
                    if (child.empty())
                        throw std::runtime_error("standard module not found: " + imp);

                } else {

                    child = imp;

                    if (child.extension().empty())
                        child += ".nx";

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
                        candidates.push_back(canon.parent_path() / "vendor" / vendorName /
                                             "src/main.nx");

                        candidates.push_back(canon.parent_path() / "vendor" / child);

                        for (const auto& c : candidates)
                            if (fs::exists(c)) {
                                child = c;
                                break;
                            }
                    }
                }
                appendProgram(all, loadProgram(child, context));
            }
            appendProgram(all, std::move(prog));
            context.loaded.insert(canon);

            return all;
        }

        fs::path resolveInput(const std::string& arg) {

            if (!arg.empty()) {

                fs::path p = arg;

                if (p.is_relative())
                    p = fs::absolute(p);

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
                candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "examples" / "templates" /
                                     templateName);
                candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "examples" / exampleName);
#endif
            }
            candidates.push_back(fs::current_path() / "examples" / "templates" / templateName);
            candidates.push_back(fs::current_path() / "examples" / exampleName);
            for (const auto& candidate : candidates) {
                std::error_code ec;
                if (fs::is_regular_file(candidate, ec))
                    return fs::weakly_canonical(candidate, ec);
            }
            return {};
        }

        fs::path runtimeSource() {
            if (const char* env = std::getenv("NEXUS_RUNTIME_DIR")) {
                fs::path candidate = fs::path(env) / "runtime.cpp";
                if (fs::is_regular_file(candidate))
                    return fs::weakly_canonical(candidate);
                candidate = fs::path(env) / "runtime.c";
                if (fs::is_regular_file(candidate))
                    return fs::weakly_canonical(candidate);
            }
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
#ifdef NEXUS_SOURCE_ROOT
            const fs::path sourceRuntime = fs::path(NEXUS_SOURCE_ROOT) / "src/runtime/runtime.cpp";
            if (fs::is_regular_file(sourceRuntime))
                return fs::weakly_canonical(sourceRuntime);
#endif
            const fs::path local = fs::current_path() / "runtime/runtime.cpp";
            if (fs::is_regular_file(local))
                return fs::weakly_canonical(local);
#ifdef NEXUS_INSTALL_RUNTIME_DIR
            const fs::path installed = fs::path(NEXUS_INSTALL_RUNTIME_DIR) / "runtime.cpp";
            if (fs::is_regular_file(installed))
                return fs::weakly_canonical(installed);
#endif
            throw std::runtime_error(
                "Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
        }

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

            if (!CreateProcessW(absoluteExecutable.c_str(),
                                commandLine.data(),
                                nullptr,
                                nullptr,
                                FALSE,
                                0,
                                nullptr,
                                absoluteWorkingDirectory.c_str(),
                                &startup,
                                &process)) {
                const DWORD error = GetLastError();
                throw std::runtime_error("cannot start native executable " +
                                         absoluteExecutable.string() + " (Windows error " +
                                         std::to_string(error) + ")");
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
            for (int attempt = 0;; ++attempt) {
                std::ofstream exe(out, std::ios::binary | std::ios::trunc);
                if (exe) {
                    exe.write(reinterpret_cast<const char*>(data),
                              static_cast<std::streamsize>(size));
                    exe.close();
                    if (exe)
                        return;
                }
                if (attempt >= 20)
                    break;
#ifdef _WIN32
                Sleep(100);
#else
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
            }
            throw std::runtime_error("cannot write " + out.string());
        }

        int compileFile(const fs::path& in,
                        const fs::path& out,
                        const std::string& target,
                        bool emitIR,
                        int opt,
                        const std::string& backend = "auto",
                        bool quiet = false) {

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

        void writeProjectManifest(const fs::path& dir,
                                  const std::string& name,
                                  const std::string& templ) {

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
                << "[assets]\ntextures = \"assets/textures\"\naudio = \"assets/audio\"\nmodels = "
                   "\"assets/models\"\nfonts = \"assets/fonts\"\nscenes = \"scenes\"\n\n"
                << "[project]\ntemplate = \"" << templ << "\"\n";
        }

        fs::path sourceDataRoot() {

#ifdef NEXUS_SOURCE_ROOT
            const fs::path sourceRoot = NEXUS_SOURCE_ROOT;

            if (fs::exists(sourceRoot / "examples/assets") ||
                fs::exists(sourceRoot / "examples/templates"))
                return sourceRoot;

#endif
            if (!gExecutable.empty()) {

                const auto bin = fs::weakly_canonical(gExecutable).parent_path();

                const fs::path candidates[] = {
                    fs::current_path(), bin, bin / "..", bin / "../..", bin / "../share/nexus"};

                for (const auto& p : candidates) {

                    if (fs::exists(p / "examples/assets") || fs::exists(p / "assets") ||
                        fs::exists(p / "examples/templates"))
                        return fs::weakly_canonical(p);
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

                {assetRoot / "beep.wav", dir / "assets/audio/beep.wav"},
                {assetRoot / "cube.obj", dir / "assets/models/cube.obj"},
                {assetRoot / "sprite_sheet.bmp", dir / "assets/textures/sprite_sheet.bmp"}};

            for (const auto& [src, dst] : assets) {

                if (fs::exists(src)) {

                    std::error_code ec;

                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
                }
            }
        }

        int initProject(const fs::path& dir,
                        const std::string& name,
                        const std::string& templ,
                        bool failIfNonEmpty) {

            if (fs::exists(dir / "nexus.toml")) {

                if (failIfNonEmpty)
                    throw std::runtime_error("project already exists: " + dir.string());

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

            if (!fs::exists(dir / "src/main.nx"))
                std::ofstream(dir / "src/main.nx") << source;

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

                cfg.root / "assets/textures",
                cfg.root / "assets/audio",
                cfg.root / "assets/models",
                cfg.root / "assets/fonts",
                cfg.root / "scenes"};

            std::size_t count = 0;

            std::cout << "Assets for " << cfg.name << ":\n";

            for (const auto& dir : dirs) {

                if (!fs::exists(dir))
                    continue;

                for (const auto& e : fs::recursive_directory_iterator(dir)) {

                    if (!e.is_regular_file())
                        continue;

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

            if (ec)
                throw std::runtime_error("cannot remove build directory: " + ec.message());

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

        void writeText(const fs::path& p, const std::string& data) {

            fs::create_directories(p.parent_path().empty() ? fs::current_path() : p.parent_path());

            std::ofstream f(p, std::ios::binary);

            if (!f)
                throw std::runtime_error("cannot write " + p.string());

            f << data;
        }

        // Write to a sibling temporary file and replace the destination only after
        // a successful flush. An interrupted write must not truncate package metadata.
        void writeTextAtomically(const fs::path& p, const std::string& data) {
            const fs::path parent = p.parent_path().empty() ? fs::current_path() : p.parent_path();
            fs::create_directories(parent);
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            fs::path temporary = p;
            temporary += ".tmp-" + std::to_string(stamp);

            {
                std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
                if (!out)
                    throw std::runtime_error("cannot create temporary metadata file " +
                                             temporary.string());
                out.write(data.data(), static_cast<std::streamsize>(data.size()));
                out.flush();
                if (!out) {
                    out.close();
                    std::error_code cleanup;
                    fs::remove(temporary, cleanup);
                    throw std::runtime_error("cannot finish writing temporary metadata file " +
                                             temporary.string());
                }
            }

#ifdef _WIN32
            if (!MoveFileExW(temporary.c_str(), p.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                const DWORD error = GetLastError();
                std::error_code cleanup;
                fs::remove(temporary, cleanup);
                throw std::runtime_error("cannot atomically replace " + p.string() +
                                         " (Windows error " + std::to_string(error) + ")");
            }
#else
            std::error_code ec;
            fs::rename(temporary, p, ec);
            if (ec) {
                std::error_code cleanup;
                fs::remove(temporary, cleanup);
                throw std::runtime_error("cannot atomically replace " + p.string() + ": " +
                                         ec.message());
            }
#endif
        }

        std::string packageTreeSha256(const fs::path& root) {
            static const std::set<std::string> excluded = {
                ".git", ".vscode", "build", "dist", "node_modules", "CMakeFiles"};
            std::vector<fs::path> files;
            std::error_code ec;
            for (fs::recursive_directory_iterator it(
                     root, fs::directory_options::skip_permission_denied, ec), end;
                 it != end; it.increment(ec)) {
                if (ec)
                    throw std::runtime_error("cannot traverse package for checksum: " + ec.message());
                const auto status = it->symlink_status(ec);
                if (ec)
                    throw std::runtime_error("cannot inspect package for checksum: " + ec.message());
                if (fs::is_symlink(status)) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_directory(status) && excluded.contains(it->path().filename().string())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_regular_file(status))
                    files.push_back(it->path());
            }

            std::sort(files.begin(), files.end(), [&](const fs::path& left, const fs::path& right) {
                return left.lexically_relative(root).generic_string() <
                       right.lexically_relative(root).generic_string();
            });
            std::string manifest;
            for (const auto& file : files) {
                manifest += file.lexically_relative(root).generic_string();
                manifest.push_back('\0');
                manifest += package::sha256Hex(readFile(file));
                manifest.push_back('\n');
            }
            return "sha256:" + package::sha256Hex(manifest);
        }

        int commandFmt(const fs::path& file, bool checkOnly = false) {
            const std::string src = readFile(file);
            const std::string formatted = formatNexusSource(src);
            const bool touched = formatted != src;
            if (touched && checkOnly) {
                std::cout << "fmt: needs formatting -> " << file << '\n';
                return 1;
            }
            if (touched)
                writeText(file, formatted);
            if (checkOnly)
                std::cout << "fmt: OK -> " << file << '\n';
            else
                std::cout << "fmt: " << (touched ? "updated" : "already formatted") << " -> "
                          << file << '\n';
            return 0;
        }

        int commandFmtAll(bool checkOnly) {
            const auto cfg = loadProject();
            static const std::set<std::string> excludedDirectories = {
                ".git", ".vscode", "build", "dist", "vendor", "node_modules",
                "CMakeFiles", "target", ".cache"};

            std::vector<fs::path> files;
            std::error_code ec;
            for (fs::recursive_directory_iterator it(
                     cfg.root, fs::directory_options::skip_permission_denied, ec), end;
                 it != end; it.increment(ec)) {
                if (ec)
                    throw std::runtime_error("cannot discover NEXUS source files: " + ec.message());

                const auto status = it->symlink_status(ec);
                if (ec)
                    throw std::runtime_error("cannot inspect project entry: " + ec.message());

                if (fs::is_symlink(status)) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_directory(status) &&
                    excludedDirectories.contains(it->path().filename().string())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (fs::is_regular_file(status) && it->path().extension() == ".nx")
                    files.push_back(it->path());
            }

            std::sort(files.begin(), files.end(),
                      [](const fs::path& left, const fs::path& right) {
                          return left.generic_string() < right.generic_string();
                      });

            if (files.empty()) {
                std::cout << "fmt: no .nx files found in " << cfg.root << '\n';
                return 0;
            }

            std::size_t failed = 0;
            for (const auto& file : files) {
                try {
                    const int result = commandFmt(file, checkOnly);
                    if (result != 0)
                        ++failed;
                } catch (const std::exception& error) {
                    ++failed;
                    std::cerr << "fmt: " << file << ": " << error.what() << '\n';
                }
            }

            if (checkOnly) {
                if (failed != 0) {
                    std::cout << "fmt: " << failed << " file(s) need formatting or could not be read\n";
                    return 1;
                }
                std::cout << "fmt: OK (" << files.size() << " file(s))\n";
                return 0;
            }

            std::cout << "fmt: processed " << files.size() << " file(s)"
                      << (failed ? ", " + std::to_string(failed) + " failure(s)" : "") << '\n';
            return failed == 0 ? 0 : 1;
        }

        int commandSelfTest() {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path scratch =
                fs::temp_directory_path() / ("nexus-self-test-" + std::to_string(stamp));
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
            if (ec)
                throw std::runtime_error("cannot create self-test directory: " + ec.message());

            try {
                writeText(source, "fn main() -> i64 { return 42 }\n");
                const int buildRc = compileFile(source, executable, "native", false, 2, backend);
                if (buildRc != 0)
                    throw std::runtime_error("compiler returned " + std::to_string(buildRc));
                const int runRc = runCommand({executable.string()});
                if (runRc != 42)
                    throw std::runtime_error("self-test expected exit code 42 but got " +
                                             std::to_string(runRc));
                fs::remove_all(scratch, ec);
                std::cout << "self-test: PASS ("
                          << (preferNativeBackend ? "built-in native"
                                                  : "configured compiler fallback")
                          << " compile + run)\n";
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

            while (std::getline(in, line)) {

                ++lineNo;

                if (!line.empty() &&
                    (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {

                    ++warnings;
                    std::cout << "warning[NX9001]: trailing whitespace at " << file << ':' << lineNo
                              << '\n';
                }
                if (line.find("TODO") != std::string::npos ||
                    line.find("FIXME") != std::string::npos) {

                    ++warnings;
                    std::cout << "warning[NX9002]: unfinished marker at " << file << ':' << lineNo
                              << '\n';
                }
            }
            ModuleLoadContext loadContext;

            auto prog = loadProgram(file, loadContext);

            SemanticAnalyzer().analyze(prog);

            std::cout << "lint: OK"
                      << (warnings ? ", " + std::to_string(warnings) + " warning(s)" : "") << '\n';

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

            while (std::getline(in, line)) {

                const auto t = trim(line);

                if (t.rfind("///", 0) == 0) {
                    pending += trim(t.substr(3)) + "\n";
                    continue;
                }
                if (t.rfind("fn ", 0) == 0 || t.rfind("struct ", 0) == 0 ||
                    t.rfind("enum ", 0) == 0) {

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

            samples.reserve(std::max(1, runs));

            for (int i = 0; i < std::max(1, runs); ++i) {

                const auto a = std::chrono::steady_clock::now();

                const int rc = runCommand({out.string()});

                const auto b = std::chrono::steady_clock::now();

                if (rc != 0)
                    throw std::runtime_error("benchmark program exited with code " +
                                             std::to_string(rc));

                samples.push_back(
                    std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
            }
            std::error_code ec;
            fs::remove(out, ec);

            long long total = 0;
            for (auto x : samples)
                total += x;

            std::cout << "bench: " << samples.size() << " run(s), average "
                      << (total / samples.size()) << " us\n";

            return 0;
        }

        int commandTest() {

            const auto cfg = loadProject();

            const auto root = cfg.root / "tests/programs";

            if (!fs::exists(root)) {
                std::cout << "test: no tests/programs directory\n";
                return 0;
            }
            std::size_t count = 0;

            for (const auto& e : fs::recursive_directory_iterator(root)) {

                if (!e.is_regular_file() || e.path().extension() != ".nx")
                    continue;

                ++count;

                const auto out =
                    fs::temp_directory_path() / (e.path().stem().string() + ".nexus-test");

                compileFile(e.path(), out, "native", false, 2);

                const int rc = runCommand({out.string()});

                std::error_code ec;
                fs::remove(out, ec);

                if (rc != 0)
                    throw std::runtime_error("test failed: " + e.path().string());

                std::cout << "  PASS " << fs::relative(e.path(), cfg.root).string() << "\n";
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
                if (a.rfind("--target=", 0) == 0)
                    target = a.substr(9);
                else if (a == "--target" && i + 1 < args.size())
                    target = args[++i];
                else if (a.rfind("--backend=", 0) == 0)
                    backend = a.substr(10);
                else if (a == "--backend" && i + 1 < args.size())
                    backend = args[++i];
                else if (a == "-o" && i + 1 < args.size())
                    explicitOutput = args[++i];
                else
                    throw std::runtime_error("unknown option: " + a);
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
                if (out.is_relative())
                    out = dist / out;
            } else {
                out = dist / cfg.artifact;
            }
            if (windowsTarget && out.extension().empty())
                out += ".exe";
            compileFile(in, out, target, false, 3, backend);

            const std::string label = target.empty() ? "native" : target;
            const auto stage = dist / (cfg.name + "-" + cfg.version);
            std::error_code ec;
            fs::remove_all(stage, ec);
            fs::create_directories(stage, ec);
            fs::copy(out, stage / out.filename(), fs::copy_options::overwrite_existing, ec);
            if (ec)
                throw std::runtime_error("cannot stage export binary: " + ec.message());
            for (const auto& rel : {std::string("assets"), std::string("scenes")}) {
                if (fs::exists(cfg.root / rel))
                    fs::copy(cfg.root / rel,
                             stage / rel,
                             fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                             ec);
            }
            if (fs::exists(cfg.root / "nexus.toml"))
                fs::copy(cfg.root / "nexus.toml",
                         stage / "nexus.toml",
                         fs::copy_options::overwrite_existing,
                         ec);
            if (ec)
                throw std::runtime_error("cannot stage export files: " + ec.message());
            writeText(stage / "EXPORT.txt",
                      "package = \"" + cfg.name + "\"\nversion = \"" + cfg.version +
                          "\"\ntarget = \"" + label + "\"\nbackend = \"" + backend + "\"\n");

            const auto archive = dist / (cfg.name + "-" + cfg.version + "-" + label + ".zip");
            fs::remove(archive, ec);
            // cmake is only needed to zip the archive; the binary and the
            // staged directory are the actual deliverables. Clean machines
            // without cmake still get a successful export.
            const int rc = runCommand({"cmake",
                                       "-E",
                                       "tar",
                                       "cf",
                                       archive.string(),
                                       "--format=zip",
                                       stage.filename().string()},
                                      dist);
            if (rc == 127) {
                std::cout << "export: binary  -> " << out << "\n"
                          << "export: staged  -> " << stage << "\n"
                          << "export: note: archive skipped (cmake not on PATH)\n";
                return 0;
            }
            if (rc != 0)
                throw std::runtime_error("cannot create export archive");
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
            if (name.empty() || name == "." || name == "..")
                return false;
            for (const unsigned char ch : name) {
                if (!std::isalnum(ch) && ch != '_' && ch != '-')
                    return false;
            }
            return true;
        }

        std::string stripTomlInlineComment(const std::string& line) {
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = 0; i < line.size(); ++i) {
                const char ch = line[i];
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                } else if (ch == '"') {
                    inString = true;
                } else if (ch == '#') {
                    return line.substr(0, i);
                }
            }
            return line;
        }

        std::size_t findTomlAssignment(const std::string& line) {
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = 0; i < line.size(); ++i) {
                const char ch = line[i];
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                } else if (ch == '"') {
                    inString = true;
                } else if (ch == '=') {
                    return i;
                }
            }
            return std::string::npos;
        }

        std::string parsePackageMetadataString(const std::string& rawValue,
                                               const std::string& context) {
            const std::string value = trim(rawValue);
            if (value.size() < 2 || value.front() != '"' || value.back() != '"')
                throw std::runtime_error(context + ": expected a quoted string");

            std::string decoded;
            decoded.reserve(value.size() - 2);
            for (std::size_t i = 1; i + 1 < value.size(); ++i) {
                const char ch = value[i];
                if (ch == '"')
                    throw std::runtime_error(context + ": unescaped quote in string");
                if (ch != '\\') {
                    if (static_cast<unsigned char>(ch) < 0x20)
                        throw std::runtime_error(context + ": control character in string");
                    decoded.push_back(ch);
                    continue;
                }

                ++i;
                if (i + 1 >= value.size())
                    throw std::runtime_error(context + ": unfinished string escape");
                switch (value[i]) {
                case '"': decoded.push_back('"'); break;
                case '\\': decoded.push_back('\\'); break;
                case 'b': decoded.push_back('\b'); break;
                case 't': decoded.push_back('\t'); break;
                case 'n': decoded.push_back('\n'); break;
                case 'f': decoded.push_back('\f'); break;
                case 'r': decoded.push_back('\r'); break;
                default:
                    throw std::runtime_error(context + ": unsupported string escape");
                }
            }
            return decoded;
        }

        std::map<std::string, std::string>
        readStrictTomlStringSection(const std::string& data,
                                    const std::string& wantedSection,
                                    const std::string& context) {
            std::map<std::string, std::string> values;
            std::set<std::string> seenTables;
            std::string activeSection;
            bool sawWantedSection = false;
            std::istringstream input(data);
            std::string line;
            std::size_t lineNumber = 0;
            while (std::getline(input, line)) {
                ++lineNumber;
                const std::string original = trim(stripTomlInlineComment(line));
                if (original.empty())
                    continue;

                if (original.front() == '[') {
                    const bool arrayTable = original.size() >= 4 &&
                        original.rfind("[[", 0) == 0 &&
                        original.substr(original.size() - 2) == "]]";
                    const bool regularTable = original.size() >= 3 &&
                        original.front() == '[' && original.back() == ']' && !arrayTable &&
                        original.find(']', 1) == original.size() - 1;
                    if (!arrayTable && !regularTable)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));

                    const std::size_t start = arrayTable ? 2 : 1;
                    const std::size_t length =
                        original.size() - start - (arrayTable ? 2 : 1);
                    const std::string section = trim(original.substr(start, length));
                    if (section.empty() || section.find('[') != std::string::npos ||
                        section.find(']') != std::string::npos)
                        throw std::runtime_error(context + ": invalid table header at line " +
                                                 std::to_string(lineNumber));
                    if (arrayTable && section == wantedSection)
                        throw std::runtime_error(context + ": [" + wantedSection +
                                                 "] must be a regular table");
                    if (regularTable && !seenTables.insert(section).second)
                        throw std::runtime_error(context + ": duplicate table [" + section + "]");
                    if (section == wantedSection) {
                        if (sawWantedSection)
                            throw std::runtime_error(context + ": duplicate table [" +
                                                     wantedSection + "]");
                        sawWantedSection = true;
                    }
                    activeSection = arrayTable ? "[[" + section + "]]" : section;
                    continue;
                }

                const std::string current = trim(stripTomlInlineComment(line));
                if (current.empty() || activeSection != wantedSection)
                    continue;
                const auto equal = findTomlAssignment(current);
                if (equal == std::string::npos)
                    throw std::runtime_error(context + ": malformed entry in [" +
                                             wantedSection + "] at line " +
                                             std::to_string(lineNumber) +
                                             ": expected key = \"value\"");

                const std::string key = trim(current.substr(0, equal));
                if (!validPackageName(key))
                    throw std::runtime_error(context + ": invalid dependency name at line " +
                                             std::to_string(lineNumber));
                const std::string value = parsePackageMetadataString(
                    current.substr(equal + 1),
                    context + " [" + wantedSection + "] line " +
                        std::to_string(lineNumber));
                if (!values.emplace(key, value).second)
                    throw std::runtime_error(context + ": duplicate key '" + key +
                                             "' in [" + wantedSection + "]");
            }
            return values;
        }

        struct ParsedPackageLockfile {
            std::map<std::string, std::string> metadata;
            std::map<std::string, std::map<std::string, std::string>> sections;
        };

        ParsedPackageLockfile parsePackageLockfile(const std::string& data) {
            constexpr const char* header = "# NEXUS lockfile v2 (SHA-256 content integrity)";
            ParsedPackageLockfile result;
            std::set<std::string> seenSections;
            std::istringstream input(data);
            std::string line;
            std::string activeSection;
            std::size_t lineNumber = 0;
            bool checkedHeader = false;

            while (std::getline(input, line)) {
                ++lineNumber;
                const std::string raw = trim(line);
                if (!checkedHeader && raw.empty())
                    continue;
                if (!checkedHeader) {
                    checkedHeader = true;
                    if (raw != header)
                        throw std::runtime_error(
                            "pkg verify: invalid lockfile: missing NEXUS lockfile v2 header");
                    continue;
                }

                const std::string current = trim(stripTomlInlineComment(line));
                if (current.empty())
                    continue;
                if (current.front() == '[') {
                    if (current.size() < 3 || current.back() != ']' ||
                        current.rfind("[[", 0) == 0 ||
                        current.find(']', 1) != current.size() - 1)
                        throw std::runtime_error("pkg verify: invalid lockfile table at line " +
                                                 std::to_string(lineNumber));
                    activeSection = trim(current.substr(1, current.size() - 2));
                    if (activeSection != "dependencies" && activeSection != "sources" &&
                        activeSection != "integrity")
                        throw std::runtime_error("pkg verify: unsupported lockfile table [" +
                                                 activeSection + "]");
                    if (!seenSections.insert(activeSection).second)
                        throw std::runtime_error("pkg verify: invalid lockfile: duplicate table [" +
                                                 activeSection + "]");
                    continue;
                }

                const auto equal = findTomlAssignment(current);
                if (equal == std::string::npos)
                    throw std::runtime_error("pkg verify: invalid lockfile entry at line " +
                                             std::to_string(lineNumber));
                const std::string key = trim(current.substr(0, equal));
                if (!validPackageName(key))
                    throw std::runtime_error("pkg verify: invalid lockfile key at line " +
                                             std::to_string(lineNumber));
                const std::string value = parsePackageMetadataString(
                    current.substr(equal + 1),
                    "pkg verify: invalid lockfile value at line " +
                        std::to_string(lineNumber));

                if (activeSection.empty()) {
                    if (key != "package" && key != "version")
                        throw std::runtime_error("pkg verify: invalid lockfile metadata key '" +
                                                 key + "'");
                    if (!result.metadata.emplace(key, value).second)
                        throw std::runtime_error("pkg verify: invalid lockfile: duplicate key '" +
                                                 key + "'");
                } else if (!result.sections[activeSection].emplace(key, value).second) {
                    throw std::runtime_error("pkg verify: invalid lockfile: duplicate key '" +
                                             key + "' in [" + activeSection + "]");
                }
            }

            if (!checkedHeader || !result.metadata.contains("package") ||
                !result.metadata.contains("version") || result.metadata.at("package").empty() ||
                result.metadata.at("version").empty())
                throw std::runtime_error(
                    "pkg verify: invalid lockfile: package and version metadata are required");
            return result;
        }

        void ensureSafeVendorDirectory(const ProjectConfig& cfg, bool createIfMissing) {
            const fs::path vendor = cfg.root / "vendor";
            std::error_code ec;
            const auto status = fs::symlink_status(vendor, ec);
            if (ec == std::errc::no_such_file_or_directory ||
                (!ec && status.type() == fs::file_type::not_found)) {
                ec.clear();
                if (createIfMissing) {
                    fs::create_directories(vendor, ec);
                    if (ec)
                        throw std::runtime_error("cannot create vendor directory: " + ec.message());
                }
                return;
            }
            if (ec)
                throw std::runtime_error("cannot inspect vendor directory: " + ec.message());
            if (fs::is_symlink(status))
                throw std::runtime_error("vendor directory must not be a symlink");
            if (!fs::is_directory(status))
                throw std::runtime_error("vendor path exists but is not a directory");

            const fs::path canonicalRoot = fs::weakly_canonical(cfg.root, ec);
            if (ec)
                throw std::runtime_error("cannot resolve project root: " + ec.message());
            const fs::path canonicalVendor = fs::canonical(vendor, ec);
            if (ec)
                throw std::runtime_error("cannot resolve vendor directory: " + ec.message());
            const fs::path relativeVendor = canonicalVendor.lexically_relative(canonicalRoot);
            if (relativeVendor != fs::path("vendor"))
                throw std::runtime_error("vendor directory resolves outside the project root");
        }

        void updateDependency(const fs::path& manifest,
                              const std::string& name,
                              const std::string& value,
                              bool remove) {
            std::istringstream input(readFile(manifest));
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(input, line))
                lines.push_back(line);
            std::size_t start = lines.size(), finish = lines.size();
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (trim(lines[i]) != "[dependencies]")
                    continue;
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
                if (remove)
                    return;
                if (!lines.empty() && !trim(lines.back()).empty())
                    lines.emplace_back();
                lines.emplace_back("[dependencies]");
                lines.emplace_back(name + " = \"" + value + "\"");
            } else {
                bool found = false;
                std::size_t at = start;
                for (; at < finish; ++at) {
                    const std::string t = trim(lines[at]);
                    if (t.empty() || t.front() == '#')
                        continue;
                    const auto eq = t.find('=');
                    if (eq == std::string::npos || trim(t.substr(0, eq)) != name)
                        continue;
                    found = true;
                    break;
                }
                if (found) {
                    if (remove)
                        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(at));
                    else
                        lines[at] = name + " = \"" + value + "\"";
                } else if (!remove) {
                    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(finish),
                                 name + " = \"" + value + "\"");
                }
            }
            std::ostringstream output;
            for (const auto& entry : lines)
                output << entry << '\n';
            writeTextAtomically(manifest, output.str());
        }

        void refreshLockfile(const ProjectConfig& cfg) {
            struct Dependency {
                std::string name;
                std::string version;
                std::string source;
                std::string integrity;
            };
            std::vector<Dependency> dependencies;
            std::istringstream input(readFile(cfg.root / "nexus.toml"));
            std::string line;
            bool inDependencies = false;
            while (std::getline(input, line)) {
                const std::string t = trim(line);
                if (t.empty() || t.front() == '#')
                    continue;
                if (t.front() == '[' && t.back() == ']') {
                    inDependencies = t == "[dependencies]";
                    continue;
                }
                if (!inDependencies)
                    continue;
                const auto eq = t.find('=');
                if (eq == std::string::npos)
                    continue;
                const std::string name = trim(t.substr(0, eq));
                std::string value = trim(t.substr(eq + 1));
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);

                Dependency dependency{name, value, {}, {}};
                const fs::path local(value);
                if (!local.is_absolute()) {
                    const fs::path dependencyRoot = (cfg.root / local).lexically_normal();
                    const fs::path rel = dependencyRoot.lexically_relative(cfg.root);
                    std::error_code ec;
                    if (!rel.empty() && *rel.begin() == "vendor" &&
                        fs::is_regular_file(dependencyRoot / "nexus.toml", ec)) {
                        try {
                            const auto dep = loadProject(dependencyRoot);
                            if (!dep.version.empty())
                                dependency.version = dep.version;
                            dependency.source = rel.generic_string();
                            dependency.integrity = packageTreeSha256(dependencyRoot);
                        } catch (const std::exception& error) {
                            throw std::runtime_error("cannot lock local dependency '" + name +
                                                     "': " + error.what());
                        }
                    }
                }
                dependencies.push_back(std::move(dependency));
            }

            std::ostringstream lock;
            lock << "# NEXUS lockfile v2 (SHA-256 content integrity)\npackage = \"" << cfg.name
                 << "\"\nversion = \"" << cfg.version << "\"\n";
            if (!dependencies.empty()) {
                lock << "\n[dependencies]\n";
                for (const auto& dep : dependencies)
                    lock << dep.name << " = \"" << dep.version << "\"\n";

                bool haveSources = false, haveIntegrity = false;
                for (const auto& dep : dependencies) {
                    haveSources = haveSources || !dep.source.empty();
                    haveIntegrity = haveIntegrity || !dep.integrity.empty();
                }
                if (haveSources) {
                    lock << "\n[sources]\n";
                    for (const auto& dep : dependencies)
                        if (!dep.source.empty())
                            lock << dep.name << " = \"" << dep.source << "\"\n";
                }
                if (haveIntegrity) {
                    lock << "\n[integrity]\n";
                    for (const auto& dep : dependencies)
                        if (!dep.integrity.empty())
                            lock << dep.name << " = \"" << dep.integrity << "\"\n";
                }
            }
            writeTextAtomically(cfg.root / "nexus.lock", lock.str());
        }

        void copyPackageTree(const fs::path& source, const fs::path& destination) {
            const std::set<std::string> skipped = {
                ".git", ".vscode", "build", "dist", "node_modules", "CMakeFiles"};
            std::error_code ec;
            fs::create_directories(destination, ec);
            if (ec)
                throw std::runtime_error("cannot create vendor directory: " + ec.message());
            try {
                for (fs::recursive_directory_iterator
                         it(source, fs::directory_options::skip_permission_denied, ec),
                     end;
                     it != end;
                     it.increment(ec)) {
                    if (ec)
                        throw std::runtime_error("cannot traverse package source: " + ec.message());
                    const auto status = it->symlink_status(ec);
                    if (ec)
                        throw std::runtime_error("cannot inspect package entry: " + ec.message());
                    if (fs::is_symlink(status))
                        continue;
                    if (fs::is_directory(status) &&
                        skipped.contains(it->path().filename().string())) {
                        it.disable_recursion_pending();
                        continue;
                    }
                    const fs::path rel = it->path().lexically_relative(source);
                    const fs::path target = destination / rel;
                    if (fs::is_directory(status))
                        fs::create_directories(target, ec);
                    else if (fs::is_regular_file(status)) {
                        fs::create_directories(target.parent_path(), ec);
                        if (!ec)
                            fs::copy_file(
                                it->path(), target, fs::copy_options::overwrite_existing, ec);
                    }
                    if (ec)
                        throw std::runtime_error("cannot copy package entry '" + rel.string() +
                                                 "': " + ec.message());
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
                std::cout << "nexus pkg init|add <path>|remove <name>|list|update|verify|build|publish\n";
                return 0;
            }
            const std::string sub = args[0];
            const fs::path vendor = cfg.root / "vendor";
            if (sub == "init" || sub == "add" || sub == "remove")
                ensureSafeVendorDirectory(cfg, true);
            else if (sub == "list" || sub == "update" || sub == "verify")
                ensureSafeVendorDirectory(cfg, false);
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
                        if (!entry.is_directory())
                            continue;
                        std::cout << "  " << entry.path().filename().string();
                        if (fs::is_regular_file(entry.path() / "nexus.toml")) {
                            try {
                                std::cout << " " << loadProject(entry.path()).version;
                            } catch (const std::exception&) {
                                std::cout << " (invalid manifest)";
                            }
                        }
                        std::cout << '\n';
                    }
                }
                return 0;
            }
            if (sub == "add") {
                if (args.size() != 2)
                    throw std::runtime_error(
                        "pkg add requires exactly one local package directory");
                fs::path source(args[1]);
                if (source.is_relative())
                    source = fs::absolute(source);
                std::error_code ec;
                source = fs::weakly_canonical(source, ec);
                if (ec || !fs::is_directory(source, ec))
                    throw std::runtime_error("pkg add source is not an accessible directory");
                if (!fs::is_regular_file(source / "nexus.toml"))
                    throw std::runtime_error("package directory must contain nexus.toml");
                const auto dep = loadProject(source);
                if (!validPackageName(dep.name))
                    throw std::runtime_error(
                        "invalid package name: use letters, digits, '_' or '-'");
                const fs::path root = fs::weakly_canonical(cfg.root, ec);
                if (ec)
                    throw std::runtime_error("cannot resolve project root: " + ec.message());
                const fs::path destination = (vendor / dep.name).lexically_normal();
                if (source == root)
                    throw std::runtime_error(
                        "cannot add the current project as its own dependency");
                const fs::path relativeDestination = destination.lexically_relative(source);
                if (!relativeDestination.empty() && *relativeDestination.begin() != "..")
                    throw std::runtime_error("package source contains the vendor destination; "
                                             "choose a separate directory");

                const fs::path manifestPath = cfg.root / "nexus.toml";
                const fs::path lockPath = cfg.root / "nexus.lock";
                const std::string manifestBefore = readFile(manifestPath);
                const auto lockStatus = fs::symlink_status(lockPath, ec);
                bool hadLockfile = false;
                if (ec == std::errc::no_such_file_or_directory ||
                    (!ec && lockStatus.type() == fs::file_type::not_found)) {
                    ec.clear();
                } else {
                    if (ec)
                        throw std::runtime_error("cannot inspect lockfile: " + ec.message());
                    if (fs::is_symlink(lockStatus) || !fs::is_regular_file(lockStatus))
                        throw std::runtime_error("nexus.lock must be a regular file, not a symlink");
                    hadLockfile = true;
                }
                const std::string lockBefore = hadLockfile ? readFile(lockPath) : std::string{};

                const auto stamp =
                    std::chrono::steady_clock::now().time_since_epoch().count();
                const fs::path backup =
                    root / (".nexus-pkg-backup-" + dep.name + "-" + std::to_string(stamp));
                if (fs::exists(backup, ec) || ec)
                    throw std::runtime_error("cannot reserve package rollback path");
                ec.clear();

                const auto destinationStatus = fs::symlink_status(destination, ec);
                const bool hadDestination =
                    !ec && destinationStatus.type() != fs::file_type::not_found;
                if (ec == std::errc::no_such_file_or_directory)
                    ec.clear();
                else if (ec)
                    throw std::runtime_error("cannot inspect existing vendored package: " +
                                             ec.message());
                if (hadDestination) {
                    fs::rename(destination, backup, ec);
                    if (ec)
                        throw std::runtime_error("cannot stage existing vendored package: " +
                                                 ec.message());
                }

                try {
                    copyPackageTree(source, destination);
                    updateDependency(manifestPath, dep.name, "vendor/" + dep.name, false);
                    refreshLockfile(cfg);
                } catch (...) {
                    std::string rollbackFailure;
                    try {
                        std::error_code rollbackError;
                        fs::remove_all(destination, rollbackError);
                        if (rollbackError)
                            throw std::runtime_error("cannot remove failed package copy: " +
                                                     rollbackError.message());
                        if (hadDestination) {
                            fs::rename(backup, destination, rollbackError);
                            if (rollbackError)
                                throw std::runtime_error("cannot restore previous package: " +
                                                         rollbackError.message());
                        }
                        writeTextAtomically(manifestPath, manifestBefore);
                        if (hadLockfile) {
                            writeTextAtomically(lockPath, lockBefore);
                        } else {
                            fs::remove(lockPath, rollbackError);
                            if (rollbackError)
                                throw std::runtime_error("cannot remove failed lockfile: " +
                                                         rollbackError.message());
                        }
                    } catch (const std::exception& rollbackError) {
                        rollbackFailure = rollbackError.what();
                    }
                    if (!rollbackFailure.empty())
                        throw std::runtime_error("pkg add failed and rollback was incomplete: " +
                                                 rollbackFailure);
                    throw;
                }

                if (hadDestination) {
                    fs::remove_all(backup, ec);
                    if (ec)
                        std::cerr << "pkg add: warning: old package backup remains at "
                                  << backup << ": " << ec.message() << '\n';
                }
                std::cout << "pkg add: vendored " << dep.name << " " << dep.version << "\n";
                return 0;
            }
            if (sub == "remove") {
                if (args.size() != 2)
                    throw std::runtime_error("pkg remove requires exactly one package name");
                const std::string name = args[1];
                if (!validPackageName(name))
                    throw std::runtime_error(
                        "invalid package name: path separators and traversal are not allowed");
                std::error_code ec;
                fs::remove_all(vendor / name, ec);
                if (ec)
                    throw std::runtime_error("cannot remove vendored package: " + ec.message());
                updateDependency(cfg.root / "nexus.toml", name, "", true);
                refreshLockfile(cfg);
                std::cout << "pkg remove: removed " << name
                          << " and refreshed nexus.toml/nexus.lock\n";
                return 0;
            }
            if (sub == "update") {
                refreshLockfile(cfg);
                std::cout << "pkg update: lockfile refreshed from nexus.toml\n";
                return 0;
            }
            if (sub == "verify") {
                const std::string manifest = readFile(cfg.root / "nexus.toml");
                const std::string lock = readFile(cfg.root / "nexus.lock");
                const auto manifestDependencies =
                    readStrictTomlStringSection(manifest, "dependencies", "nexus.toml");
                const auto parsedLock = parsePackageLockfile(lock);
                static const std::map<std::string, std::string> emptyLockSection;
                const auto lockSection = [&](const std::string& name)
                    -> const std::map<std::string, std::string>& {
                    const auto section = parsedLock.sections.find(name);
                    return section == parsedLock.sections.end() ? emptyLockSection
                                                                 : section->second;
                };
                const auto& lockedDependencies = lockSection("dependencies");
                const auto& sources = lockSection("sources");
                const auto& integrities = lockSection("integrity");

                bool failed = false;
                std::size_t verified = 0;
                std::error_code rootError;
                const fs::path canonicalRoot = fs::weakly_canonical(cfg.root, rootError);
                if (rootError)
                    throw std::runtime_error("cannot resolve project root for package verification");

                if (parsedLock.metadata.at("package") != cfg.name) {
                    std::cerr << "pkg verify: lockfile mismatch: package name differs from nexus.toml\n";
                    failed = true;
                }
                if (parsedLock.metadata.at("version") != cfg.version) {
                    std::cerr << "pkg verify: lockfile mismatch: project version differs from nexus.toml\n";
                    failed = true;
                }

                // Reject stale lockfiles even if every previously locked package
                // still hashes correctly.
                for (const auto& [name, value] : manifestDependencies) {
                    (void)value;
                    if (!lockedDependencies.contains(name)) {
                        std::cerr << "pkg verify: lockfile mismatch: dependency '" << name
                                  << "' is absent from nexus.lock\n";
                        failed = true;
                    }
                }
                for (const auto& [name, version] : lockedDependencies) {
                    (void)version;
                    if (!manifestDependencies.contains(name)) {
                        std::cerr << "pkg verify: lockfile mismatch: stale dependency '" << name
                                  << "' is not declared in nexus.toml\n";
                        failed = true;
                    }
                }

                // Dependencies whose manifest path is inside vendor/ are local
                // packages and require source and integrity records.
                for (const auto& [name, value] : manifestDependencies) {
                    const fs::path declared(value);
                    const fs::path normalized = declared.lexically_normal();
                    if (!declared.is_absolute() && normalized.begin() != normalized.end() &&
                        *normalized.begin() == "vendor") {
                        if (!sources.contains(name)) {
                            std::cerr << "pkg verify: lockfile mismatch: missing source for local "
                                      << "dependency '" << name << "'\n";
                            failed = true;
                        }
                        if (!integrities.contains(name)) {
                            std::cerr << "pkg verify: lockfile mismatch: missing checksum for local "
                                      << "dependency '" << name << "'\n";
                            failed = true;
                        }
                        const fs::path localRoot = (cfg.root / normalized).lexically_normal();
                        std::error_code localError;
                        if (!fs::is_regular_file(localRoot / "nexus.toml", localError)) {
                            std::cerr << "pkg verify: missing local package for dependency '"
                                      << name << "'\n";
                            failed = true;
                        }
                    }
                }

                for (const auto& [name, source] : sources) {
                    const auto integrity = integrities.find(name);
                    if (integrity == integrities.end()) {
                        std::cerr << "pkg verify: missing checksum for " << name << "\n";
                        failed = true;
                        continue;
                    }
                    const auto declaredDependency = manifestDependencies.find(name);
                    if (declaredDependency == manifestDependencies.end()) {
                        std::cerr << "pkg verify: lockfile mismatch: source for undeclared dependency '"
                                  << name << "'\n";
                        failed = true;
                        continue;
                    }

                    const fs::path relative(source);
                    const fs::path declared(declaredDependency->second);
                    if (relative.is_absolute() || relative.empty() ||
                        relative.begin() == relative.end() || *relative.begin() != "vendor" ||
                        declared.is_absolute() ||
                        declared.lexically_normal().generic_string() !=
                            relative.lexically_normal().generic_string()) {
                        std::cerr << "pkg verify: unsafe or stale source path for " << name << "\n";
                        failed = true;
                        continue;
                    }

                    std::error_code vendorError;
                    const fs::path canonicalVendor =
                        fs::weakly_canonical(canonicalRoot / "vendor", vendorError);
                    if (vendorError) {
                        std::cerr << "pkg verify: cannot resolve vendor directory for " << name
                                  << "\n";
                        failed = true;
                        continue;
                    }
                    const fs::path vendorInsideRoot =
                        canonicalVendor.lexically_relative(canonicalRoot);
                    if (vendorInsideRoot.empty() || *vendorInsideRoot.begin() == "..") {
                        std::cerr << "pkg verify: vendor directory escapes project for " << name
                                  << "\n";
                        failed = true;
                        continue;
                    }

                    const fs::path dependency = (canonicalRoot / relative).lexically_normal();
                    std::error_code dependencyError;
                    const fs::path canonicalDependency =
                        fs::weakly_canonical(dependency, dependencyError);
                    if (dependencyError || !fs::is_directory(canonicalDependency) ||
                        !fs::is_regular_file(canonicalDependency / "nexus.toml")) {
                        std::cerr << "pkg verify: missing local package " << name << "\n";
                        failed = true;
                        continue;
                    }

                    // Project-root containment is insufficient: vendor/../other is
                    // still inside the project. Require the resolved path to remain
                    // strictly below the canonical vendor directory (also rejecting
                    // a vendor symlink that points outside the project).
                    const fs::path insideVendor =
                        canonicalDependency.lexically_relative(canonicalVendor);
                    if (insideVendor.empty() || *insideVendor.begin() == ".." ||
                        *insideVendor.begin() == ".") {
                        std::cerr << "pkg verify: package path escapes vendor for " << name << "\n";
                        failed = true;
                        continue;
                    }

                    try {
                        const auto dependencyConfig = loadProject(canonicalDependency);
                        const auto lockedVersion = lockedDependencies.find(name);
                        if (lockedVersion == lockedDependencies.end() ||
                            dependencyConfig.version != lockedVersion->second) {
                            std::cerr << "pkg verify: lockfile mismatch: version changed for "
                                      << name << "\n";
                            failed = true;
                            continue;
                        }
                    } catch (const std::exception& error) {
                        std::cerr << "pkg verify: invalid package '" << name << "': "
                                  << error.what() << "\n";
                        failed = true;
                        continue;
                    }

                    const std::string actual = packageTreeSha256(canonicalDependency);
                    if (actual != integrity->second) {
                        std::cerr << "pkg verify: integrity mismatch for " << name << "\n";
                        failed = true;
                        continue;
                    }
                    ++verified;
                    std::cout << "  verified " << name << " [" << actual << "]\n";
                }
                for (const auto& [name, checksum] : integrities) {
                    (void)checksum;
                    if (!sources.contains(name)) {
                        std::cerr << "pkg verify: checksum has no source entry for " << name << "\n";
                        failed = true;
                    }
                }
                if (sources.size() != integrities.size()) {
                    std::cerr << "pkg verify: source/checksum records are inconsistent\n";
                    failed = true;
                }
                if (failed)
                    return 1;
                if (sources.empty() && integrities.empty()) {
                    std::cout << "pkg verify: no checksummed local dependencies\n";
                    return 0;
                }
                std::cout << "pkg verify: OK (" << verified << " local package(s))\n";
                return 0;
            }
            if (sub == "build") {
                const auto input = cfg.root / cfg.entry;
                const std::string triple = targetTriple(cfg.target);
                bool windowsTarget = false;
#ifdef _WIN32
                windowsTarget = cfg.target.empty() || cfg.target == "native" ||
                                triple.find("windows") != std::string::npos;
#else
                windowsTarget = triple.find("windows") != std::string::npos;
#endif
                const auto output = defaultOutput(input, "", true, windowsTarget);
                compileFile(input, output, cfg.target, false, 3);
                std::cout << "pkg build: OK -> " << output << "\n";
                return 0;
            }
            if (sub == "publish") {
                if (!validPackageName(cfg.name))
                    throw std::runtime_error("invalid package name in nexus.toml");
                const auto dist = cfg.root / "dist";
                fs::create_directories(dist);
                const auto archive = dist / (cfg.name + "-" + cfg.version + ".nxpkg");
                std::vector<std::string> tarArgv = {
                    "cmake", "-E", "tar", "cf", archive.string(), "--format=zip"};
                for (const auto& relative : {std::string("nexus.toml"),
                                             std::string("src"),
                                             std::string("assets"),
                                             std::string("scenes"),
                                             std::string("vendor"),
                                             std::string("docs")})
                    if (fs::exists(cfg.root / relative))
                        tarArgv.push_back(relative);
                const int rc = runCommand(tarArgv, cfg.root);
                if (rc != 0)
                    throw std::runtime_error("cannot create package archive");
                std::cout << "pkg publish: created local archive " << archive
                          << " (no remote registry upload is configured)\n";
                return 0;
            }
            throw std::runtime_error("unknown pkg command: " + sub);
        }

        int commandLsp() {
#ifdef _WIN32
            // LSP framing uses explicit CRLF bytes; disable Windows text-mode newline translation.
            if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
                std::cerr << "NEXUS LSP: could not switch stdout to binary mode\\n";
                return 2;
            }
#endif
            const auto extractString = [](const std::string& json, const std::string& field) {
                const std::string key = "\"" + field + "\"";
                auto pos = json.find(key);
                if (pos == std::string::npos)
                    return std::string{};
                pos = json.find(':', pos + key.size());
                if (pos == std::string::npos)
                    return std::string{};
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                    ++pos;
                if (pos >= json.size() || json[pos] != '"')
                    return std::string{};
                std::string value;
                for (++pos; pos < json.size(); ++pos) {
                    const char c = json[pos];
                    if (c == '"')
                        return value;
                    if (c == '\\' && pos + 1 < json.size()) {
                        const char escaped = json[++pos];
                        switch (escaped) {
                        case 'n':
                            value.push_back('\n');
                            break;
                        case 'r':
                            value.push_back('\r');
                            break;
                        case 't':
                            value.push_back('\t');
                            break;
                        default:
                            value.push_back(escaped);
                            break;
                        }
                    } else {
                        value.push_back(c);
                    }
                }
                return std::string{};
            };
            const auto extractId = [](const std::string& json) {
                const auto key = json.find("\"id\"");
                if (key == std::string::npos)
                    return std::string{};
                auto pos = json.find(':', key + 4);
                if (pos == std::string::npos)
                    return std::string{};
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                    ++pos;
                if (pos >= json.size())
                    return std::string{};
                const auto begin = pos;
                if (json[pos] == '"') {
                    ++pos;
                    bool escaped = false;
                    while (pos < json.size()) {
                        if (!escaped && json[pos] == '"') {
                            ++pos;
                            break;
                        }
                        if (!escaped && json[pos] == '\\')
                            escaped = true;
                        else
                            escaped = false;
                        ++pos;
                    }
                } else {
                    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' &&
                           !std::isspace(static_cast<unsigned char>(json[pos])))
                        ++pos;
                }
                return json.substr(begin, pos - begin);
            };
            const auto extractNumber = [](const std::string& json, const std::string& field) {
                const std::string key = "\"" + field + "\"";
                const auto keyPos = json.find(key);
                if (keyPos == std::string::npos)
                    return std::size_t{0};
                auto pos = json.find(':', keyPos + key.size());
                if (pos == std::string::npos)
                    return std::size_t{0};
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
                    ++pos;
                const auto begin = pos;
                while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos])))
                    ++pos;
                if (begin == pos)
                    return std::size_t{0};
                try {
                    return static_cast<std::size_t>(std::stoull(json.substr(begin, pos - begin)));
                } catch (const std::exception&) {
                    return std::size_t{0};
                }
            };
            const auto supportsWatchedFilesRegistration = [](const std::string& json) {
                const auto workspace = json.find("\"workspace\"");
                if (workspace == std::string::npos)
                    return false;
                const auto watchedFiles = json.find("\"didChangeWatchedFiles\"", workspace);
                if (watchedFiles == std::string::npos)
                    return false;
                const auto dynamicRegistration = json.find("\"dynamicRegistration\"", watchedFiles);
                if (dynamicRegistration == std::string::npos)
                    return false;
                const auto colon = json.find(':', dynamicRegistration);
                if (colon == std::string::npos)
                    return false;
                const auto value = json.find_first_not_of(" \t\r\n", colon + 1);
                return value != std::string::npos && json.compare(value, 4, "true") == 0;
            };
            bool clientSupportsWatchedFilesRegistration = false;
            const auto send = [](const std::string& payload) {
                std::cout << "Content-Length: " << payload.size() << "\r\n\r\n"
                          << payload << std::flush;
            };
            const auto jsonEscape = [](const std::string& value) {
                std::string escaped;
                for (const unsigned char c : value) {
                    switch (c) {
                    case '"':
                        escaped += "\\\"";
                        break;
                    case '\\':
                        escaped += "\\\\";
                        break;
                    case '\n':
                        escaped += "\\n";
                        break;
                    case '\r':
                        escaped += "\\r";
                        break;
                    case '\t':
                        escaped += "\\t";
                        break;
                    case '\b':
                        escaped += "\\b";
                        break;
                    case '\f':
                        escaped += "\\f";
                        break;
                    default:
                        if (c < 0x20) {
                            constexpr char hex[] = "0123456789abcdef";
                            escaped += "\\u00";
                            escaped.push_back(hex[(c >> 4) & 0x0f]);
                            escaped.push_back(hex[c & 0x0f]);
                        } else
                            escaped.push_back(static_cast<char>(c));
                        break;
                    }
                }
                return escaped;
            };
            struct LspDiagnostic {
                std::size_t line = 0;
                std::size_t character = 0;
                std::size_t length = 1;
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
                    diagnostic.message =
                        marker == std::string::npos ? raw : trim(raw.substr(0, marker));
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
                const auto applyTokenRanges = [&]() {
                    const auto utf16Length = [](const std::string& text) {
                        std::size_t units = 0;
                        for (std::size_t i = 0; i < text.size();) {
                            const auto ch = static_cast<unsigned char>(text[i]);
                            std::size_t width = 1;
                            if ((ch & 0xe0) == 0xc0)
                                width = 2;
                            else if ((ch & 0xf0) == 0xe0)
                                width = 3;
                            else if ((ch & 0xf8) == 0xf0)
                                width = 4;
                            if (i + width > text.size())
                                width = 1;
                            units += width == 4 ? 2 : 1;
                            i += width;
                        }
                        return units;
                    };
                    for (auto& diagnostic : diagnostics) {
                        std::string target;
                        const auto quote = diagnostic.message.find('\'');
                        if (diagnostic.message.rfind("unknown variable: ", 0) == 0) {
                            target = diagnostic.message.substr(std::string("unknown variable: ").size());
                        } else if (diagnostic.message.find("field") != std::string::npos &&
                                   quote != std::string::npos) {
                            const auto endQuote = diagnostic.message.find('\'', quote + 1);
                            if (endQuote != std::string::npos && endQuote > quote + 1)
                                target = diagnostic.message.substr(quote + 1, endQuote - quote - 1);
                        } else if (diagnostic.message.rfind("unknown enum variant: ", 0) == 0) {
                            const auto separator = diagnostic.message.rfind("::");
                            if (separator != std::string::npos)
                                target = diagnostic.message.substr(separator + 2);
                        }
                        if (!target.empty()) {
                            const Token* closest = nullptr;
                            std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
                            for (const auto& token : diagnosticTokens) {
                                if (token.kind != TokenKind::Identifier || token.text != target)
                                    continue;
                                const std::size_t distance =
                                    token.pos.line == diagnostic.line + 1
                                        ? (token.pos.column > diagnostic.character + 1
                                               ? token.pos.column - diagnostic.character - 1
                                               : diagnostic.character + 1 - token.pos.column)
                                        : std::numeric_limits<std::size_t>::max() / 2 +
                                              (token.pos.line > diagnostic.line + 1
                                                   ? token.pos.line - diagnostic.line - 1
                                                   : diagnostic.line + 1 - token.pos.line);
                                if (distance < bestDistance) {
                                    bestDistance = distance;
                                    closest = &token;
                                }
                            }
                            if (closest) {
                                diagnostic.line = closest->pos.line - 1;
                                diagnostic.character = closest->pos.column - 1;
                            }
                        }
                        for (const auto& token : diagnosticTokens) {
                            if (token.text.empty() || token.kind == TokenKind::End ||
                                token.pos.line != diagnostic.line + 1 ||
                                token.pos.column != diagnostic.character + 1)
                                continue;
                            diagnostic.length = std::max<std::size_t>(1, utf16Length(token.text));
                            const std::size_t offset = std::min(token.pos.offset, source.size());
                            const auto newline = source.rfind('\n', offset == 0 ? 0 : offset - 1);
                            const std::size_t lineStart =
                                newline == std::string::npos ? 0 : newline + 1;
                            diagnostic.character =
                                utf16Length(source.substr(lineStart, offset - lineStart));
                            break;
                        }
                    }
                };
                Program program;
                try {
                    program = Parser(std::move(tokens)).parseProgram();
                } catch (const std::exception& error) {
                    const std::string message = error.what();
                    if (message.rfind("NEXUS found ", 0) == 0 &&
                        message.find(" syntax error") != std::string::npos) {
                        std::istringstream errors(message);
                        std::string line;
                        std::getline(errors, line); // Skip the aggregate summary.
                        while (std::getline(errors, line)) {
                            if (line.rfind("  - ", 0) != 0)
                                continue;
                            const std::runtime_error one(line.substr(4));
                            add(one, "NX1001");
                        }
                        if (diagnostics.empty())
                            add(error, "NX1001");
                    } else {
                        add(error, "NX1001");
                    }
                    applyTokenRanges();
                    return diagnostics;
                }
                try {
                    SemanticAnalyzer().analyze(program);
                } catch (const std::exception& error) {
                    const std::string message = error.what();
                    if (message.rfind("NEXUS found ", 0) == 0 &&
                        message.find(" semantic error") != std::string::npos) {
                        std::istringstream errors(message);
                        std::string line;
                        std::getline(errors, line);
                        while (std::getline(errors, line)) {
                            if (line.rfind("  - ", 0) != 0)
                                continue;
                            const std::runtime_error one(line.substr(4));
                            add(one, "NX2001");
                        }
                        if (diagnostics.empty())
                            add(error, "NX2001");
                    } else {
                        add(error, "NX2001");
                    }
                }
                applyTokenRanges();
                return diagnostics;
            };
            std::unordered_map<std::string, std::string> openDocuments;
            // Decode the path component of a file:// URI, including UTF-8 bytes.
            const auto pathFromUri = [](const std::string& uri) -> fs::path {
                if (uri.rfind("file://", 0) != 0)
                    return {};
                const std::string encoded = uri.substr(7);
                std::string decoded;
                decoded.reserve(encoded.size());
                const auto hexValue = [](char c) -> int {
                    if (c >= '0' && c <= '9')
                        return c - '0';
                    if (c >= 'a' && c <= 'f')
                        return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F')
                        return c - 'A' + 10;
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
                     (decoded[1] >= 'a' && decoded[1] <= 'z')) &&
                    decoded[2] == ':') {
                    decoded.erase(decoded.begin());
                }
#endif
                return fs::path(decoded);
            };
            const auto uriFromPath = [](const fs::path& inputPath) {
                std::error_code ec;
                fs::path absolute = fs::absolute(inputPath, ec);
                if (ec)
                    absolute = inputPath;
                std::string path = absolute.lexically_normal().generic_string();
                if (path.empty() || path.front() != '/')
                    path.insert(path.begin(), '/');
                static constexpr char hex[] = "0123456789ABCDEF";
                std::string encoded;
                encoded.reserve(path.size());
                for (const unsigned char ch : path) {
                    const bool unreserved = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                                            (ch >= '0' && ch <= '9') || ch == '/' || ch == ':' ||
                                            ch == '-' || ch == '_' || ch == '.' || ch == '~';
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
            struct LspTypeDeclarationIndex {
                std::string source;
                std::set<std::string> structNames;
                std::set<std::string> enumNames;
                std::set<std::string> functionNames;
                std::vector<StructDecl> structs;
                std::vector<EnumDecl> enums;
                std::map<std::string, std::string> functionReturnTypes;
                bool declarationsParsed = false;
            };
            std::map<std::string, LspTypeDeclarationIndex> workspaceTypeDeclarationCache;
            const auto workspaceRootForUri = [&](const std::string& uri) -> fs::path {
                const fs::path documentPath = pathFromUri(uri);
                if (documentPath.empty())
                    return {};
                const fs::path projectRoot = findProjectRoot(documentPath);
                return projectRoot.empty() ? documentPath.parent_path() : projectRoot;
            };
            const auto workspaceCacheKeyForUri = [&](const std::string& uri) {
                const fs::path root = workspaceRootForUri(uri);
                if (root.empty() || root == root.root_path())
                    return std::string{};
                std::error_code ec;
                const fs::path normalized = fs::weakly_canonical(root, ec);
                return (ec ? root.lexically_normal() : normalized).generic_string();
            };
            const auto collectWorkspaceDocuments = [&](const std::string& requestUri,
                                                       const std::string& requestSource) {
                std::map<std::string, std::string> byUri;
                const fs::path documentPath = pathFromUri(requestUri);
                const fs::path projectRoot =
                    documentPath.empty() ? fs::path{} : findProjectRoot(documentPath);
                const bool hasProjectManifest = !projectRoot.empty();
                const fs::path root =
                    documentPath.empty()
                        ? fs::path{}
                        : (hasProjectManifest ? projectRoot : documentPath.parent_path());

                // Virtual editor URIs and root-level paths only participate through
                // currently open buffers. Never walk the filesystem root.
                const bool canScan = !root.empty() && root != root.root_path();
                const std::string cacheKey =
                    canScan ? workspaceCacheKeyForUri(requestUri) : std::string{};
                if (canScan && !cacheKey.empty()) {
                    auto cached = workspaceDiskCache.find(cacheKey);
                    if (cached == workspaceDiskCache.end()) {
                        std::map<std::string, std::string> diskDocuments;
                        std::size_t indexedFiles = 0;
                        constexpr std::size_t maxWorkspaceFiles = 10000;
                        std::error_code ec;
                        const auto addFile = [&](const fs::path& path) {
                            if (indexedFiles >= maxWorkspaceFiles || path.extension() != ".nx")
                                return;
                            const std::string fileUri = uriFromPath(path);
                            if (diskDocuments.find(fileUri) != diskDocuments.end())
                                return;
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
                                        name == "target" || name == "node_modules" ||
                                        name == ".nexus") {
                                        it.disable_recursion_pending();
                                    }
                                } else if (it->is_regular_file(ec)) {
                                    addFile(current);
                                }
                                it.increment(ec);
                            }
                        } else {
                            // Without nexus.toml, discover only direct .nx siblings.
                            fs::directory_iterator it(
                                root, fs::directory_options::skip_permission_denied, ec),
                                end;
                            while (!ec && it != end && indexedFiles < maxWorkspaceFiles) {
                                if (it->is_regular_file(ec))
                                    addFile(it->path());
                                it.increment(ec);
                            }
                        }
                        cached =
                            workspaceDiskCache.emplace(cacheKey, std::move(diskDocuments)).first;
                    }
                    byUri = cached->second;
                }

                // Unsaved editor buffers override cached disk content. Include all
                // open buffers so cross-file navigation also works for virtual docs.
                for (const auto& opened : openDocuments)
                    byUri[opened.first] = opened.second;
                byUri[requestUri] = requestSource;

                std::vector<std::pair<std::string, std::string>> result;
                result.reserve(byUri.size());
                for (const auto& item : byUri)
                    result.push_back(item);
                return result;
            };
            const auto utf16Length = [](const std::string& text) {
                std::size_t units = 0;
                for (std::size_t i = 0; i < text.size();) {
                    const unsigned char ch = static_cast<unsigned char>(text[i]);
                    std::size_t width = 1;
                    if ((ch & 0xe0) == 0xc0)
                        width = 2;
                    else if ((ch & 0xf0) == 0xe0)
                        width = 3;
                    else if ((ch & 0xf8) == 0xf0)
                        width = 4;
                    if (i + width > text.size())
                        width = 1;
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
                    if ((ch & 0xe0) == 0xc0)
                        width = 2;
                    else if ((ch & 0xf0) == 0xe0)
                        width = 3;
                    else if ((ch & 0xf8) == 0xf0)
                        width = 4;
                    if (i + width > text.size())
                        width = 1;
                    const std::size_t step = width == 4 ? 2 : 1;
                    if (units + step > wanted)
                        return i;
                    if (units + step == wanted)
                        return i + width;
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
            // Contextual completion scans tokens first, and parses only source files that
            // declare the requested type or function. This keeps editor requests cheap.
            const auto workspaceTypeIndexFor =
                [&](const std::string& documentUri,
                    const std::string& source) -> LspTypeDeclarationIndex& {
                    auto cached = workspaceTypeDeclarationCache.find(documentUri);
                    if (cached == workspaceTypeDeclarationCache.end() ||
                        cached->second.source != source) {
                        LspTypeDeclarationIndex index;
                        index.source = source;
                        try {
                            const auto tokens = Lexer(source).tokenize();
                            for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
                                if (tokens[i + 1].kind != TokenKind::Identifier)
                                    continue;
                                if (tokens[i].kind == TokenKind::KwStruct)
                                    index.structNames.insert(tokens[i + 1].text);
                                else if (tokens[i].kind == TokenKind::KwEnum)
                                    index.enumNames.insert(tokens[i + 1].text);
                                else if (tokens[i].kind == TokenKind::KwFn)
                                    index.functionNames.insert(tokens[i + 1].text);
                            }
                        } catch (const std::exception&) {
                            // One incomplete workspace buffer must not break completion.
                        }
                        cached = workspaceTypeDeclarationCache
                                     .insert_or_assign(documentUri, std::move(index))
                                     .first;
                    }
                    return cached->second;
                };
            const auto parseWorkspaceTypeIndex = [&](LspTypeDeclarationIndex& index) {
                if (index.declarationsParsed)
                    return;
                try {
                    Program parsed = Parser(Lexer(index.source).tokenize()).parseProgram();
                    index.structs = std::move(parsed.structs);
                    index.enums = std::move(parsed.enums);
                    for (const auto& function : parsed.functions) {
                        // Member completion needs named struct return types. Primitive
                        // returns intentionally do not produce field suggestions.
                        if (function.ret.kind == TypeKind::Struct &&
                            !function.ret.name.empty()) {
                            index.functionReturnTypes.insert_or_assign(
                                function.name, function.ret.name);
                        }
                    }
                } catch (const std::exception&) {
                    // A malformed source file contributes no contextual declarations.
                    index.structs.clear();
                    index.enums.clear();
                    index.functionReturnTypes.clear();
                }
                index.declarationsParsed = true;
            };
            const auto workspaceTypeDeclarationsFor =
                [&](const std::string& requestUri,
                    const std::string& requestSource,
                    const std::string& requestedType,
                    bool requestedEnum) {
                    std::pair<std::vector<StructDecl>, std::vector<EnumDecl>> result;
                    if (requestedType.empty())
                        return result;

                    for (const auto& [documentUri, source] :
                         collectWorkspaceDocuments(requestUri, requestSource)) {
                        if (documentUri == requestUri)
                            continue;

                        auto& index = workspaceTypeIndexFor(documentUri, source);
                        const bool typeExists = requestedEnum
                                                    ? index.enumNames.contains(requestedType)
                                                    : index.structNames.contains(requestedType);
                        if (!typeExists)
                            continue;

                        parseWorkspaceTypeIndex(index);
                        if (requestedEnum) {
                            for (const auto& declaration : index.enums) {
                                if (declaration.name == requestedType)
                                    result.second.push_back(declaration);
                            }
                        } else {
                            for (const auto& declaration : index.structs) {
                                if (declaration.name == requestedType)
                                    result.first.push_back(declaration);
                            }
                        }
                    }
                    return result;
                };
            const auto workspaceFunctionReturnTypeFor =
                [&](const std::string& requestUri,
                    const std::string& requestSource,
                    const std::string& requestedFunction) {
                    if (requestedFunction.empty())
                        return std::string{};

                    for (const auto& [documentUri, source] :
                         collectWorkspaceDocuments(requestUri, requestSource)) {
                        if (documentUri == requestUri)
                            continue;

                        auto& index = workspaceTypeIndexFor(documentUri, source);
                        if (!index.functionNames.contains(requestedFunction))
                            continue;

                        parseWorkspaceTypeIndex(index);
                        const auto found = index.functionReturnTypes.find(requestedFunction);
                        if (found != index.functionReturnTypes.end())
                            return found->second;
                    }
                    return std::string{};
                };

            const auto identifierAtPosition = [&](const std::string& source,
                                                  std::size_t requestedLine,
                                                  std::size_t requestedCharacter) {
                std::istringstream input(source);
                std::vector<std::string> sourceLines;
                std::string line;
                while (std::getline(input, line))
                    sourceLines.push_back(line);
                if (requestedLine >= sourceLines.size())
                    return std::string{};
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
                            token.pos.line != requestedLine + 1 || token.text.empty())
                            continue;
                        bool wordToken = true;
                        for (const unsigned char ch : token.text) {
                            if (!identifierCharacter(ch)) {
                                wordToken = false;
                                break;
                            }
                        }
                        if (!wordToken)
                            continue;
                        const std::size_t start = token.pos.column > 0 ? token.pos.column - 1 : 0;
                        if (cursor >= start && cursor < start + token.text.size())
                            return token.text;
                    }
                } catch (const std::exception&) {
                    // An incomplete string/comment can make the current buffer un-tokenizable.
                }
                return std::string{};
            };
            const auto findIdentifierOccurrences = [&](const std::string& identifier,
                                                       const std::string& requestUri,
                                                       const std::string& requestSource,
                                                       std::size_t requestedLine,
                                                       std::size_t requestedCharacter) {
                std::vector<LspIdentifierOccurrence> found;
                if (identifier.empty())
                    return found;

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
                            if (scopeStack.size() > 1)
                                scopeStack.pop_back();
                        } else {
                            indexed.tokenScope[i] = scopeStack.back();
                        }
                    }

                    const auto addDeclaration = [&](std::size_t tokenIndex,
                                                    std::size_t visibleFrom,
                                                    std::size_t scope,
                                                    const std::string& kind,
                                                    bool global) {
                        if (tokenIndex >= tokenCount ||
                            indexed.tokens[tokenIndex].kind != TokenKind::Identifier)
                            return;
                        const std::string name = indexed.tokens[tokenIndex].text;
                        const std::string identity =
                            global ? "global:" + kind + ":" + name
                                   : "local:" + indexed.uri + ":" + std::to_string(tokenIndex);
                        indexed.declarations.push_back(
                            {name, identity, kind, tokenIndex, visibleFrom, scope, global});
                    };

                    for (std::size_t i = 1; i < tokenCount; ++i) {
                        if (indexed.tokenScope[i] != 0 ||
                            indexed.tokens[i].kind != TokenKind::Identifier)
                            continue;
                        const TokenKind before = indexed.tokens[i - 1].kind;
                        if (before == TokenKind::KwFn)
                            addDeclaration(i, 0, 0, "function", true);
                        else if (before == TokenKind::KwStruct)
                            addDeclaration(i, 0, 0, "struct", true);
                        else if (before == TokenKind::KwEnum)
                            addDeclaration(i, 0, 0, "enum", true);
                    }

                    for (std::size_t i = 0; i < tokenCount; ++i) {
                        const TokenKind kind = indexed.tokens[i].kind;
                        if (kind == TokenKind::KwLet || kind == TokenKind::KwConst) {
                            std::size_t nameIndex = i + 1;
                            if (kind == TokenKind::KwLet && nameIndex < tokenCount &&
                                indexed.tokens[nameIndex].kind == TokenKind::KwMut)
                                ++nameIndex;
                            if (nameIndex < tokenCount &&
                                indexed.tokens[nameIndex].kind == TokenKind::Identifier) {
                                addDeclaration(nameIndex,
                                               nameIndex,
                                               indexed.tokenScope[nameIndex],
                                               kind == TokenKind::KwConst ? "constant" : "variable",
                                               false);
                            }
                        } else if (kind == TokenKind::KwFor && i + 1 < tokenCount &&
                                   indexed.tokens[i + 1].kind == TokenKind::Identifier) {
                            const std::size_t nameIndex = i + 1;
                            std::size_t bodyOpen = nameIndex + 1;
                            while (bodyOpen < tokenCount &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::Semicolon)
                                ++bodyOpen;
                            if (bodyOpen < tokenCount &&
                                indexed.tokens[bodyOpen].kind == TokenKind::LBrace &&
                                indexed.braceScope[bodyOpen] != static_cast<std::size_t>(-1)) {
                                addDeclaration(nameIndex,
                                               bodyOpen + 1,
                                               indexed.braceScope[bodyOpen],
                                               "variable",
                                               false);
                            }
                        } else if (kind == TokenKind::KwFn) {
                            std::size_t openParen = i + 1;
                            while (openParen < tokenCount &&
                                   indexed.tokens[openParen].kind != TokenKind::LParen &&
                                   indexed.tokens[openParen].kind != TokenKind::Semicolon)
                                ++openParen;
                            if (openParen >= tokenCount ||
                                indexed.tokens[openParen].kind != TokenKind::LParen)
                                continue;
                            std::size_t closeParen = openParen + 1;
                            std::size_t parenDepth = 1;
                            for (; closeParen < tokenCount && parenDepth > 0; ++closeParen) {
                                if (indexed.tokens[closeParen].kind == TokenKind::LParen)
                                    ++parenDepth;
                                else if (indexed.tokens[closeParen].kind == TokenKind::RParen)
                                    --parenDepth;
                            }
                            if (parenDepth != 0)
                                continue;
                            --closeParen;
                            std::size_t bodyOpen = closeParen + 1;
                            while (bodyOpen < tokenCount &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::Semicolon)
                                ++bodyOpen;
                            if (bodyOpen >= tokenCount ||
                                indexed.tokens[bodyOpen].kind != TokenKind::LBrace ||
                                indexed.braceScope[bodyOpen] == static_cast<std::size_t>(-1))
                                continue;
                            const std::size_t bodyScope = indexed.braceScope[bodyOpen];
                            for (std::size_t param = openParen + 1; param + 1 < closeParen;
                                 ++param) {
                                if (indexed.tokens[param].kind == TokenKind::Identifier &&
                                    indexed.tokens[param + 1].kind == TokenKind::Colon) {
                                    addDeclaration(
                                        param, bodyOpen + 1, bodyScope, "parameter", false);
                                }
                            }
                        }
                    }
                    auto indexedEntry =
                        workspaceSymbolIndexCache.insert_or_assign(indexed.uri, std::move(indexed))
                            .first;
                    documents.push_back(&indexedEntry->second);
                }

                const auto isAncestorScope =
                    [](const LspDocumentIndex& document, std::size_t ancestor, std::size_t scope) {
                        while (scope != static_cast<std::size_t>(-1)) {
                            if (scope == ancestor)
                                return true;
                            if (scope >= document.scopes.size())
                                break;
                            scope = document.scopes[scope].parent;
                        }
                        return false;
                    };
                const auto declarationAt = [](const LspDocumentIndex& document,
                                              std::size_t tokenIndex) -> const LspSymbolDecl* {
                    for (const auto& declaration : document.declarations)
                        if (declaration.tokenIndex == tokenIndex)
                            return &declaration;
                    return nullptr;
                };
                const auto localIdentityAt = [&](const LspDocumentIndex& document,
                                                 std::size_t tokenIndex) {
                    if (tokenIndex >= document.tokens.size())
                        return std::string{};
                    if (const auto* declaration = declarationAt(document, tokenIndex);
                        declaration && !declaration->global)
                        return declaration->identity;
                    if (document.tokens[tokenIndex].kind != TokenKind::Identifier)
                        return std::string{};
                    const std::string& name = document.tokens[tokenIndex].text;
                    const LspSymbolDecl* selected = nullptr;
                    for (const auto& declaration : document.declarations) {
                        if (declaration.global || declaration.name != name)
                            continue;
                        const bool isOwnDeclaration = declaration.tokenIndex == tokenIndex;
                        const bool visible = tokenIndex >= declaration.visibleFrom &&
                                             isAncestorScope(document,
                                                             declaration.scope,
                                                             document.tokenScope[tokenIndex]);
                        if (!isOwnDeclaration && !visible)
                            continue;
                        if (!selected ||
                            document.scopes[declaration.scope].depth >
                                document.scopes[selected->scope].depth ||
                            (document.scopes[declaration.scope].depth ==
                                 document.scopes[selected->scope].depth &&
                             declaration.visibleFrom > selected->visibleFrom))
                            selected = &declaration;
                    }
                    return selected ? selected->identity : std::string{};
                };
                const auto isMemberOrFieldName = [&](const LspDocumentIndex& document,
                                                     std::size_t tokenIndex) {
                    if (tokenIndex >= document.tokens.size())
                        return true;
                    const auto& tokens = document.tokens;
                    if (tokenIndex > 0 && (tokens[tokenIndex - 1].kind == TokenKind::Dot ||
                                           tokens[tokenIndex - 1].kind == TokenKind::ColonColon))
                        return true;
                    if (tokenIndex + 1 < tokens.size() &&
                        tokens[tokenIndex + 1].kind == TokenKind::Colon && tokenIndex > 0 &&
                        (tokens[tokenIndex - 1].kind == TokenKind::LBrace ||
                         tokens[tokenIndex - 1].kind == TokenKind::Comma)) {
                        const auto* declaration = declarationAt(document, tokenIndex);
                        if (!declaration || declaration->kind != "parameter")
                            return true;
                    }
                    return false;
                };
                const auto tokenAtPosition =
                    [&](const LspDocumentIndex& document, std::size_t line, std::size_t character) {
                        std::istringstream sourceStream(document.source);
                        std::vector<std::string> sourceLines;
                        std::string sourceLine;
                        while (std::getline(sourceStream, sourceLine))
                            sourceLines.push_back(sourceLine);
                        if (line >= sourceLines.size())
                            return document.tokens.size();
                        std::size_t cursor = byteOffsetForUtf16(sourceLines[line], character);
                        if (cursor > 0 &&
                            (cursor == sourceLines[line].size() ||
                             !identifierCharacter(
                                 static_cast<unsigned char>(sourceLines[line][cursor]))) &&
                            identifierCharacter(
                                static_cast<unsigned char>(sourceLines[line][cursor - 1])))
                            --cursor;
                        for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                            const Token& token = document.tokens[i];
                            if (token.kind != TokenKind::Identifier || token.pos.line != line + 1 ||
                                token.text != identifier || token.pos.column == 0)
                                continue;
                            const std::size_t tokenStart = token.pos.column - 1;
                            if (cursor >= tokenStart && cursor < tokenStart + token.text.size())
                                return i;
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
                if (!requestedDocument)
                    return found;
                const std::size_t selectedTokenIndex =
                    tokenAtPosition(*requestedDocument, requestedLine, requestedCharacter);
                if (selectedTokenIndex >= requestedDocument->tokens.size())
                    return found;

                const LspSymbolDecl* selectedDeclaration =
                    declarationAt(*requestedDocument, selectedTokenIndex);
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
                            tokens[selectedTokenIndex + 1].kind == TokenKind::LParen)
                            preferredKind = "function";
                        else if (selectedTokenIndex + 1 < tokens.size() &&
                                 tokens[selectedTokenIndex + 1].kind == TokenKind::LBrace)
                            preferredKind = "struct";
                        else if (selectedTokenIndex + 1 < tokens.size() &&
                                 tokens[selectedTokenIndex + 1].kind == TokenKind::ColonColon)
                            preferredKind = "enum";
                        const LspSymbolDecl* global = nullptr;
                        for (const auto* documentPointer : documents) {
                            const auto& document = *documentPointer;
                            for (const auto& declaration : document.declarations) {
                                if (!declaration.global || declaration.name != identifier)
                                    continue;
                                if (!preferredKind.empty() && declaration.kind != preferredKind)
                                    continue;
                                if (!global)
                                    global = &declaration;
                            }
                            if (global)
                                break;
                        }
                        if (global) {
                            targetIdentity = global->identity;
                            targetIsGlobal = true;
                            targetKind = global->kind;
                        }
                    }
                }
                if (targetIdentity.empty())
                    return found;

                for (const auto* documentPointer : documents) {
                    const auto& document = *documentPointer;
                    for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                        const Token& token = document.tokens[i];
                        if (token.kind != TokenKind::Identifier || token.text != identifier)
                            continue;
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
                        if (!matches)
                            continue;
                        if (document.uri == requestedDocument->uri && i == selectedTokenIndex) {
                            // Keep the cursor token even for unusual valid syntax.
                        }
                        std::istringstream sourceStream(document.source);
                        std::string sourceLine;
                        for (std::size_t currentLine = 1;
                             currentLine < token.pos.line && std::getline(sourceStream, sourceLine);
                             ++currentLine) {}
                        if (token.pos.line == 0)
                            continue;
                        const std::size_t lineIndex = token.pos.line - 1;
                        const std::size_t startByte =
                            token.pos.column > 0 ? token.pos.column - 1 : 0;
                        sourceLine.clear();
                        std::istringstream lineStream(document.source);
                        for (std::size_t currentLine = 0;
                             currentLine <= lineIndex && std::getline(lineStream, sourceLine);
                             ++currentLine) {
                            if (currentLine == lineIndex)
                                break;
                        }
                        const std::size_t startCharacter = utf16Length(
                            sourceLine.substr(0, std::min(startByte, sourceLine.size())));
                        const LspSymbolDecl* occurrenceDeclaration = declarationAt(document, i);
                        const bool isDeclaration =
                            occurrenceDeclaration &&
                            occurrenceDeclaration->identity == targetIdentity;
                        found.push_back({document.uri,
                                         lineIndex,
                                         startCharacter,
                                         startCharacter + utf16Length(token.text),
                                         isDeclaration});
                    }
                }
                return found;
            };
            const auto isValidIdentifier = [&](const std::string& candidate) {
                try {
                    const auto tokens = Lexer(candidate).tokenize();
                    return tokens.size() == 2 && tokens[0].kind == TokenKind::Identifier &&
                           tokens[0].text == candidate && tokens[1].kind == TokenKind::End;
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
                    if (i)
                        payload << ',';
                    const auto& diagnostic = diagnostics[i];
                    payload << "{\"range\":{\"start\":{\"line\":" << diagnostic.line
                            << ",\"character\":" << diagnostic.character
                            << "},\"end\":{\"line\":" << diagnostic.line
                            << ",\"character\":" << diagnostic.character +
                                   std::max<std::size_t>(1, diagnostic.length)
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
                {"gfx_present", "Present the graphics frame"}};
            std::string header;
            while (std::cin) {
                std::size_t contentLength = 0;
                bool sawHeader = false;
                while (std::getline(std::cin, header)) {
                    if (!header.empty() && header.back() == '\r')
                        header.pop_back();
                    if (header.empty())
                        break;
                    if (header.rfind("Content-Length:", 0) == 0) {
                        const std::string value = trim(header.substr(15));
                        contentLength = static_cast<std::size_t>(std::stoull(value));
                        sawHeader = true;
                    }
                }
                if (!sawHeader)
                    break;
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
                if (method == "exit")
                    break;
                if (method == "initialize" && !id.empty()) {
                    clientSupportsWatchedFilesRegistration = supportsWatchedFilesRegistration(body);
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"result\":{\"serverInfo\":{\"name\":\"nexus-lsp\",\"version\":\"" +
                         kVersion +
                         "\"},\"capabilities\":{\"textDocumentSync\":1,"
                         "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":["
                         "\".\",\":\"]},"
                         "\"definitionProvider\":true,\"referencesProvider\":true,"
                         "\"renameProvider\":true,\"hoverProvider\":true,"
                         "\"documentFormattingProvider\":true,\"documentRangeFormattingProvider\":true,"
                         "\"codeActionProvider\":{\"codeActionKinds\":[\"quickfix\"]}}}}");
                } else if (method == "initialized") {
                    if (clientSupportsWatchedFilesRegistration) {
                        send("{\"jsonrpc\":\"2.0\",\"id\":\"nexus-register-nx-watcher\","
                             "\"method\":\"client/"
                             "registerCapability\",\"params\":{\"registrations\":[{"
                             "\"id\":\"nexus-watch-nx-workspace\","
                             "\"method\":\"workspace/didChangeWatchedFiles\","
                             "\"registerOptions\":{\"watchers\":[{\"globPattern\":\"**/"
                             "*.nx\",\"kind\":7}]}}]}}");
                    }
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
                    workspaceSymbolIndexCache.erase(uri);
                    workspaceTypeDeclarationCache.erase(uri);
                    const auto document = openDocuments.find(uri);
                    if (document != openDocuments.end())
                        publishDiagnostics(uri, document->second);
                } else if (method == "workspace/didChangeWatchedFiles") {
                    // Rebuild source snapshots and discard stale indexes, including
                    // deleted/renamed paths no longer present in the workspace walk.
                    workspaceDiskCache.clear();
                    workspaceSymbolIndexCache.clear();
                    workspaceTypeDeclarationCache.clear();
                } else if (method == "textDocument/didClose") {
                    const std::string uri = extractString(body, "uri");
                    if (!uri.empty()) {
                        openDocuments.erase(uri);
                        workspaceDiskCache.clear();
                        workspaceSymbolIndexCache.erase(uri);
                        workspaceTypeDeclarationCache.erase(uri);
                        send("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
                             "\"params\":{\"uri\":\"" +
                             jsonEscape(uri) + "\",\"diagnostics\":[]}}");
                    }

                } else if (method == "textDocument/hover" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::string result = "null";
                    if (document != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(document->second,
                                                 extractNumber(body, "line"),
                                                 extractNumber(body, "character"));
                        for (const auto& item : completions) {
                            if (item.first == identifier) {
                                const std::string markdown =
                                    "**" + item.first + "**\n\n" + item.second;
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
                                Program program =
                                    Parser(Lexer(document->second).tokenize()).parseProgram();
                                const auto describeType = [&](const auto& self,
                                                              const Type& type) -> std::string {
                                    switch (type.kind) {
                                    case TypeKind::I64:
                                        return "i64";
                                    case TypeKind::F64:
                                        return "f64";
                                    case TypeKind::Bool:
                                        return "bool";
                                    case TypeKind::String:
                                        return "string";
                                    case TypeKind::Void:
                                        return "void";
                                    case TypeKind::Pointer:
                                        return type.pointee ? "*" + self(self, *type.pointee)
                                                            : "*unknown";
                                    case TypeKind::Struct:
                                        return type.name.empty() ? "struct" : type.name;
                                    case TypeKind::Enum:
                                        return type.name.empty() ? "enum" : type.name;
                                    case TypeKind::Array:
                                        return type.element
                                                   ? "[" + self(self, *type.element) + "; " +
                                                         std::to_string(type.arraySize) + "]"
                                                   : "[unknown]";
                                    case TypeKind::Unknown:
                                        return "inferred";
                                    }
                                    return "unknown";
                                };
                                std::string markdown;
                                for (const auto& function : program.functions) {
                                    if (function.name != identifier)
                                        continue;
                                    std::ostringstream signature;
                                    signature << "fn " << function.name << "(";
                                    for (std::size_t i = 0; i < function.params.size(); ++i) {
                                        if (i)
                                            signature << ", ";
                                        signature
                                            << function.params[i].name << ": "
                                            << describeType(describeType, function.params[i].type);
                                    }
                                    signature << ") -> "
                                              << describeType(describeType, function.ret);
                                    markdown =
                                        "**" + signature.str() + "**\n\nUser-defined function.";
                                    break;
                                }
                                if (markdown.empty()) {
                                    for (const auto& declaration : program.structs) {
                                        if (declaration.name != identifier)
                                            continue;
                                        std::ostringstream description;
                                        description << "**struct " << declaration.name << "**";
                                        if (declaration.fields.empty()) {
                                            description << "\n\nNo fields.";
                                        } else {
                                            description << "\n\nFields:";
                                            for (const auto& field : declaration.fields) {
                                                description
                                                    << "\n- `" << field.name << "`: "
                                                    << describeType(describeType, field.type)
                                                    << "`";
                                            }
                                        }
                                        markdown = description.str();
                                        break;
                                    }
                                }
                                if (markdown.empty()) {
                                    for (const auto& declaration : program.enums) {
                                        if (declaration.name != identifier)
                                            continue;
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
                } else if ((method == "textDocument/completion" || method == "completion") &&
                           !id.empty()) {
                    std::vector<std::pair<std::string, std::string>> choices;
                    std::string prefix, objectName, enumName;
                    bool memberContext = false, enumContext = false;
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::size_t currentLine = 0, currentCharacter = 0, absoluteCursor = 0;
                    std::string currentLineText;
                    if (document != openDocuments.end()) {
                        currentLine = extractNumber(body, "line");
                        currentCharacter = extractNumber(body, "character");
                        const std::string& source = document->second;
                        std::size_t lineStart = 0;
                        for (std::size_t i = 0; i < currentLine; ++i) {
                            const auto newline = source.find('\n', lineStart);
                            if (newline == std::string::npos) {
                                lineStart = source.size();
                                break;
                            }
                            lineStart = newline + 1;
                        }
                        if (lineStart <= source.size()) {
                            const auto newline = source.find('\n', lineStart);
                            currentLineText = source.substr(
                                lineStart, newline == std::string::npos
                                               ? std::string::npos
                                               : newline - lineStart);
                            const std::size_t cursor =
                                std::min(byteOffsetForUtf16(currentLineText, currentCharacter),
                                         currentLineText.size());
                            absoluteCursor = lineStart + cursor;
                            std::size_t prefixStart = cursor;
                            while (prefixStart > 0 &&
                                   identifierCharacter(static_cast<unsigned char>(
                                       currentLineText[prefixStart - 1])))
                                --prefixStart;
                            prefix = currentLineText.substr(prefixStart, cursor - prefixStart);

                            if (prefixStart > 0 && currentLineText[prefixStart - 1] == '.') {
                                memberContext = true;
                                std::size_t objectEnd = prefixStart - 1;
                                while (objectEnd > 0 &&
                                       std::isspace(static_cast<unsigned char>(
                                           currentLineText[objectEnd - 1])))
                                    --objectEnd;
                                std::size_t objectStart = objectEnd;
                                while (objectStart > 0 &&
                                       identifierCharacter(static_cast<unsigned char>(
                                           currentLineText[objectStart - 1])))
                                    --objectStart;
                                objectName =
                                    currentLineText.substr(objectStart, objectEnd - objectStart);
                            } else if (prefixStart >= 2 &&
                                       currentLineText.substr(prefixStart - 2, 2) == "::") {
                                enumContext = true;
                                const std::size_t separator = prefixStart - 2;
                                std::size_t typeStart = separator;
                                while (typeStart > 0 &&
                                       identifierCharacter(static_cast<unsigned char>(
                                           currentLineText[typeStart - 1])))
                                    --typeStart;
                                enumName = currentLineText.substr(typeStart, separator - typeStart);
                            }
                        }
                    }

                    bool contextualSuggestions = false;
                    if (document != openDocuments.end() && (memberContext || enumContext)) {
                        try {
                            std::string parseSource = document->second;
                            parseSource.insert(absoluteCursor, "__nexus_completion");
                            Program program = Parser(Lexer(parseSource).tokenize()).parseProgram();

                            if (enumContext) {
                                std::vector<EnumDecl> enumDeclarations = program.enums;
                                const bool hasLocalEnum =
                                    std::any_of(enumDeclarations.begin(),
                                                enumDeclarations.end(),
                                                [&](const EnumDecl& declaration) {
                                                    return declaration.name == enumName;
                                                });
                                if (!hasLocalEnum) {
                                    auto workspaceTypes = workspaceTypeDeclarationsFor(
                                        uri, document->second, enumName, true);
                                    enumDeclarations.insert(enumDeclarations.end(),
                                                            workspaceTypes.second.begin(),
                                                            workspaceTypes.second.end());
                                }
                                for (const auto& declaration : enumDeclarations) {
                                    if (declaration.name != enumName)
                                        continue;
                                    for (const auto& variant : declaration.variants) {
                                        const std::string label = declaration.name + "::" + variant;
                                        if (prefix.empty() || label.rfind(enumName + "::" + prefix, 0) == 0)
                                            choices.emplace_back(label, "Variant of " + enumName);
                                    }
                                }
                                contextualSuggestions = !choices.empty();
                            } else {
                                std::map<std::string, std::string> variableTypes;
                                for (const auto& function : program.functions) {
                                    for (const auto& parameter : function.params) {
                                        if (!parameter.type.name.empty())
                                            variableTypes[parameter.name] = parameter.type.name;
                                    }
                                }
                                const auto collectTypes = [&](const auto& self, const Block& block) -> void {
                                    for (const auto& statement : block.statements) {
                                        if (const auto* declaration =
                                                dynamic_cast<const LetStmt*>(statement.get())) {
                                            std::string typeName = declaration->type.name;
                                            if (const auto* init =
                                                    dynamic_cast<const StructInitExpr*>(declaration->init.get()))
                                                typeName = init->typeName;
                                            else if (const auto* init =
                                                         dynamic_cast<const EnumVariantExpr*>(
                                                             declaration->init.get()))
                                                typeName = init->enumName;
                                            else if (const auto* call =
                                                         dynamic_cast<const CallExpr*>(
                                                             declaration->init.get())) {
                                                for (const auto& function : program.functions) {
                                                    if (function.name == call->callee &&
                                                        function.ret.kind == TypeKind::Struct &&
                                                        !function.ret.name.empty()) {
                                                        typeName = function.ret.name;
                                                        break;
                                                    }
                                                }
                                                if (typeName.empty()) {
                                                    typeName = workspaceFunctionReturnTypeFor(
                                                        uri, document->second, call->callee);
                                                }
                                            }
                                            if (!typeName.empty())
                                                variableTypes[declaration->name] = typeName;
                                        } else if (const auto* conditional =
                                                       dynamic_cast<const IfStmt*>(statement.get())) {
                                            if (conditional->thenBlock)
                                                self(self, *conditional->thenBlock);
                                            if (conditional->elseBlock)
                                                self(self, *conditional->elseBlock);
                                        } else if (const auto* loop =
                                                       dynamic_cast<const WhileStmt*>(statement.get())) {
                                            if (loop->body)
                                                self(self, *loop->body);
                                        } else if (const auto* match =
                                                       dynamic_cast<const MatchStmt*>(statement.get())) {
                                            for (const auto& arm : match->arms)
                                                if (arm.body)
                                                    self(self, *arm.body);
                                        } else if (const auto* nested =
                                                       dynamic_cast<const Block*>(statement.get())) {
                                            self(self, *nested);
                                        }
                                    }
                                };
                                for (const auto& function : program.functions)
                                    if (function.body)
                                        collectTypes(collectTypes, *function.body);

                                const auto type = variableTypes.find(objectName);
                                if (type != variableTypes.end()) {
                                    std::vector<StructDecl> structureDeclarations =
                                        program.structs;
                                    const bool hasLocalStruct =
                                        std::any_of(structureDeclarations.begin(),
                                                    structureDeclarations.end(),
                                                    [&](const StructDecl& declaration) {
                                                        return declaration.name == type->second;
                                                    });
                                    const bool builtinType =
                                        type->second == "i64" || type->second == "int" ||
                                        type->second == "isize" || type->second == "usize" ||
                                        type->second == "f64" || type->second == "float" ||
                                        type->second == "bool" || type->second == "string" ||
                                        type->second == "void";
                                    if (!hasLocalStruct && !builtinType) {
                                        auto workspaceTypes = workspaceTypeDeclarationsFor(
                                            uri, document->second, type->second, false);
                                        structureDeclarations.insert(structureDeclarations.end(),
                                                                      workspaceTypes.first.begin(),
                                                                      workspaceTypes.first.end());
                                    }
                                    for (const auto& declaration : structureDeclarations) {
                                        if (declaration.name != type->second)
                                            continue;
                                        for (const auto& field : declaration.fields) {
                                            if (prefix.empty() || field.name.rfind(prefix, 0) == 0)
                                                choices.emplace_back(
                                                    field.name, "Field of " + declaration.name);
                                        }
                                    }
                                    contextualSuggestions = !choices.empty();
                                }
                            }
                        } catch (const std::exception&) {
                            // Incomplete edits are normal in an editor; fall back to prefix
                            // completion rather than surfacing parser errors from this request.
                        }
                    }

                    if (!contextualSuggestions) {
                        for (const auto& item : completions) {
                            if (prefix.empty() || item.first.rfind(prefix, 0) == 0)
                                choices.push_back(item);
                        }
                    }

                    std::ostringstream result;
                    result << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":[";
                    for (std::size_t i = 0; i < choices.size(); ++i) {
                        if (i)
                            result << ',';
                        const int kind = choices[i].second.rfind("Field of ", 0) == 0
                                             ? 5
                                             : (choices[i].second.rfind("Variant of ", 0) == 0 ? 20 : 14);
                        result << "{\"label\":\"" << jsonEscape(choices[i].first)
                               << "\",\"kind\":" << kind
                               << ",\"detail\":\"" << jsonEscape(choices[i].second) << "\"}";
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
                                     "\"end\":{\"line\":" +
                                     std::to_string(endLine) +
                                     ",\"character\":" + std::to_string(endCharacter) +
                                     "}},\"newText\":\"" + jsonEscape(formatted) + "\"}]";
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");

                } else if (method == "textDocument/rangeFormatting" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto doc = openDocuments.find(uri);
                    std::string result = "[]";
                    if (doc != openDocuments.end()) {
                        const std::size_t rangePos = body.find("\"range\"");
                        const std::size_t startPos =
                            rangePos == std::string::npos ? std::string::npos
                                                          : body.find("\"start\"", rangePos);
                        const std::size_t endPos =
                            startPos == std::string::npos ? std::string::npos
                                                          : body.find("\"end\"", startPos + 7);
                        const auto nestedNumber = [&](std::size_t objectPos,
                                                       const std::string& field) {
                            if (objectPos == std::string::npos)
                                return std::size_t{0};
                            const std::size_t objectEnd = body.find('}', objectPos);
                            const std::string key = "\"" + field + "\"";
                            const std::size_t keyPos = body.find(key, objectPos);
                            if (keyPos == std::string::npos ||
                                (objectEnd != std::string::npos && keyPos >= objectEnd))
                                return std::size_t{0};
                            const std::size_t colon = body.find(':', keyPos + key.size());
                            if (colon == std::string::npos ||
                                (objectEnd != std::string::npos && colon >= objectEnd))
                                return std::size_t{0};
                            std::size_t pos = colon + 1;
                            while (pos < body.size() &&
                                   std::isspace(static_cast<unsigned char>(body[pos])))
                                ++pos;
                            const std::size_t begin = pos;
                            while (pos < body.size() &&
                                   std::isdigit(static_cast<unsigned char>(body[pos])))
                                ++pos;
                            if (begin == pos)
                                return std::size_t{0};
                            try {
                                return static_cast<std::size_t>(
                                    std::stoull(body.substr(begin, pos - begin)));
                            } catch (const std::exception&) {
                                return std::size_t{0};
                            }
                        };

                        if (startPos != std::string::npos && endPos != std::string::npos) {
                            const std::size_t firstLine = nestedNumber(startPos, "line");
                            const std::size_t requestedEndLine = nestedNumber(endPos, "line");
                            const std::size_t requestedEndCharacter =
                                nestedNumber(endPos, "character");

                            const auto splitLines = [](const std::string& text) {
                                std::vector<std::string> lines;
                                std::size_t start = 0;
                                for (;;) {
                                    const std::size_t newline = text.find('\n', start);
                                    if (newline == std::string::npos) {
                                        lines.push_back(text.substr(start));
                                        break;
                                    }
                                    lines.push_back(text.substr(start, newline - start));
                                    start = newline + 1;
                                    if (start == text.size()) {
                                        lines.emplace_back();
                                        break;
                                    }
                                }
                                for (auto& line : lines) {
                                    if (!line.empty() && line.back() == '\r')
                                        line.pop_back();
                                }
                                return lines;
                            };

                            const std::string& source = doc->second;
                            const std::string formatted = formatNexusSource(source);
                            const auto sourceLines = splitLines(source);
                            auto formattedLines = splitLines(formatted);
                            if (!source.empty() && source.back() != '\n' &&
                                formattedLines.size() == sourceLines.size() + 1 &&
                                formattedLines.back().empty()) {
                                formattedLines.pop_back();
                            }

                            if (formattedLines.size() == sourceLines.size() &&
                                firstLine < sourceLines.size() &&
                                requestedEndLine < sourceLines.size() &&
                                firstLine <= requestedEndLine) {
                                std::size_t lastLine = requestedEndLine;
                                if (lastLine > firstLine && requestedEndCharacter == 0)
                                    --lastLine;

                                const auto joinRange = [](const std::vector<std::string>& lines,
                                                           std::size_t first,
                                                           std::size_t last) {
                                    std::string text;
                                    for (std::size_t line = first; line <= last; ++line) {
                                        if (line != first)
                                            text.push_back('\n');
                                        text += lines[line];
                                    }
                                    return text;
                                };

                                const std::string oldText =
                                    joinRange(sourceLines, firstLine, lastLine);
                                const std::string newText =
                                    joinRange(formattedLines, firstLine, lastLine);
                                if (oldText != newText) {
                                    result = "[{\"range\":{\"start\":{\"line\":" +
                                             std::to_string(firstLine) +
                                             ",\"character\":0},\"end\":{\"line\":" +
                                             std::to_string(lastLine) +
                                             ",\"character\":" +
                                             std::to_string(utf16Length(sourceLines[lastLine])) +
                                             "}},\"newText\":\"" + jsonEscape(newText) + "\"}]";
                                }
                            }
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "textDocument/codeAction" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::ostringstream actions;
                    actions << '[';
                    bool firstAction = true;
                    if (document != openDocuments.end()) {
                        try {
                            const auto program =
                                Parser(Lexer(document->second).tokenize()).parseProgram();
                            const auto diagnostics = collectDiagnostics(document->second);
                            const auto editDistance = [](const std::string& left,
                                                         const std::string& right) {
                                std::vector<std::size_t> previous(right.size() + 1);
                                for (std::size_t j = 0; j <= right.size(); ++j)
                                    previous[j] = j;
                                for (std::size_t i = 1; i <= left.size(); ++i) {
                                    std::vector<std::size_t> current(right.size() + 1);
                                    current[0] = i;
                                    for (std::size_t j = 1; j <= right.size(); ++j) {
                                        const std::size_t substitution =
                                            previous[j - 1] + (left[i - 1] == right[j - 1] ? 0 : 1);
                                        current[j] = std::min({previous[j] + 1,
                                                               current[j - 1] + 1,
                                                               substitution});
                                    }
                                    previous.swap(current);
                                }
                                return previous.back();
                            };

                            for (const auto& diagnostic : diagnostics) {
                                if (diagnostic.code != "NX2001")
                                    continue;

                                const std::string variablePrefix = "unknown variable: ";
                                if (diagnostic.message.rfind(variablePrefix, 0) == 0) {
                                    const std::string misspelled =
                                        trim(diagnostic.message.substr(variablePrefix.size()));
                                    (void)findIdentifierOccurrences(misspelled,
                                                                    uri,
                                                                    document->second,
                                                                    diagnostic.line,
                                                                    diagnostic.character);
                                    const auto cached = workspaceSymbolIndexCache.find(uri);
                                    if (cached != workspaceSymbolIndexCache.end() &&
                                        cached->second.source == document->second) {
                                        const auto& index = cached->second;
                                        std::size_t useToken = index.tokens.size();
                                        for (std::size_t i = 0; i < index.tokens.size(); ++i) {
                                            const auto& token = index.tokens[i];
                                            if (token.kind == TokenKind::Identifier &&
                                                token.text == misspelled &&
                                                token.pos.line == diagnostic.line + 1 &&
                                                token.pos.column > 0 &&
                                                token.pos.column - 1 == diagnostic.character) {
                                                useToken = i;
                                                break;
                                            }
                                        }

                                        if (useToken < index.tokens.size()) {
                                            const auto isAncestor = [&](std::size_t ancestor,
                                                                        std::size_t scope) {
                                                while (scope != static_cast<std::size_t>(-1) &&
                                                       scope < index.scopes.size()) {
                                                    if (scope == ancestor)
                                                        return true;
                                                    scope = index.scopes[scope].parent;
                                                }
                                                return false;
                                            };
                                            std::set<std::string> candidates;
                                            const std::size_t useScope = index.tokenScope[useToken];
                                            for (const auto& declaration : index.declarations) {
                                                if (declaration.global ||
                                                    (declaration.kind != "variable" &&
                                                     declaration.kind != "constant" &&
                                                     declaration.kind != "parameter") ||
                                                    declaration.name == misspelled ||
                                                    declaration.visibleFrom > useToken ||
                                                    !isAncestor(declaration.scope, useScope))
                                                    continue;
                                                candidates.insert(declaration.name);
                                            }

                                            std::string bestName;
                                            std::size_t bestDistance =
                                                std::numeric_limits<std::size_t>::max();
                                            bool ambiguous = false;
                                            for (const auto& candidate : candidates) {
                                                const std::size_t distance =
                                                    editDistance(misspelled, candidate);
                                                if (distance < bestDistance) {
                                                    bestDistance = distance;
                                                    bestName = candidate;
                                                    ambiguous = false;
                                                } else if (distance == bestDistance &&
                                                           candidate != bestName) {
                                                    ambiguous = true;
                                                }
                                            }
                                            const std::size_t maxDistance =
                                                misspelled.size() <= 4 ? 1 : 2;
                                            if (!bestName.empty() && bestName != misspelled &&
                                                !ambiguous && bestDistance <= maxDistance) {
                                                if (!firstAction)
                                                    actions << ',';
                                                firstAction = false;
                                                const std::size_t length =
                                                    std::max<std::size_t>(1, diagnostic.length);
                                                actions << "{\"title\":\"Replace '"
                                                        << jsonEscape(misspelled) << "' with '"
                                                        << jsonEscape(bestName)
                                                        << "'\",\"kind\":\"quickfix\",\"edit\":{"
                                                           "\"changes\":{\""
                                                        << jsonEscape(uri)
                                                        << "\":[{\"range\":{\"start\":{\"line\":"
                                                        << diagnostic.line << ",\"character\":"
                                                        << diagnostic.character
                                                        << "},\"end\":{\"line\":"
                                                        << diagnostic.line << ",\"character\":"
                                                        << diagnostic.character + length
                                                        << "}},\"newText\":\""
                                                        << jsonEscape(bestName) << "\"}]}}}";
                                            }
                                        }
                                    }
                                    continue;
                                }

                                const std::string prefix = "unknown field '";
                                const auto start = diagnostic.message.find(prefix);
                                if (start == std::string::npos)
                                    continue;
                                const auto nameStart = start + prefix.size();
                                const auto nameEnd = diagnostic.message.find('\'', nameStart);
                                if (nameEnd == std::string::npos || nameEnd == nameStart)
                                    continue;
                                const std::string misspelled =
                                    diagnostic.message.substr(nameStart, nameEnd - nameStart);
                                std::string requiredType;
                                const auto onType = diagnostic.message.rfind(" on ");
                                if (onType != std::string::npos)
                                    requiredType = diagnostic.message.substr(onType + 4);

                                std::string bestField;
                                std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
                                bool ambiguous = false;
                                for (const auto& structure : program.structs) {
                                    if (!requiredType.empty() && structure.name != requiredType)
                                        continue;
                                    for (const auto& field : structure.fields) {
                                        const auto distance = editDistance(misspelled, field.name);
                                        if (distance < bestDistance) {
                                            bestDistance = distance;
                                            bestField = field.name;
                                            ambiguous = false;
                                        } else if (distance == bestDistance && field.name != bestField) {
                                            ambiguous = true;
                                        }
                                    }
                                }
                                const std::size_t maxDistance = misspelled.size() <= 4 ? 1 : 2;
                                if (bestField.empty() || bestField == misspelled || ambiguous ||
                                    bestDistance > maxDistance)
                                    continue;

                                if (!firstAction)
                                    actions << ',';
                                firstAction = false;
                                const std::size_t length = std::max<std::size_t>(1, diagnostic.length);
                                actions << "{\"title\":\"Replace '" << jsonEscape(misspelled)
                                        << "' with '" << jsonEscape(bestField)
                                        << "'\",\"kind\":\"quickfix\",\"edit\":{\"changes\":{\""
                                        << jsonEscape(uri) << "\":[{\"range\":{\"start\":{\"line\":"
                                        << diagnostic.line << ",\"character\":" << diagnostic.character
                                        << "},\"end\":{\"line\":" << diagnostic.line
                                        << ",\"character\":" << diagnostic.character + length
                                        << "}},\"newText\":\"" << jsonEscape(bestField) << "\"}]}}}";
                            }
                        } catch (const std::exception&) {
                            // Quick fixes are best-effort; malformed or incomplete source
                            // must not crash the language server.
                        }
                    }
                    actions << ']';
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"result\":" + actions.str() + "}");
                } else if (method == "textDocument/references" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::string result = "[]";
                    if (document != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(document->second,
                                                 extractNumber(body, "line"),
                                                 extractNumber(body, "character"));
                        const auto occurrences =
                            findIdentifierOccurrences(identifier,
                                                      uri,
                                                      document->second,
                                                      extractNumber(body, "line"),
                                                      extractNumber(body, "character"));
                        std::ostringstream payload;
                        payload << "[";
                        for (std::size_t i = 0; i < occurrences.size(); ++i) {
                            if (i)
                                payload << ',';
                            const auto& occurrence = occurrences[i];
                            payload << "{\"uri\":\"" << jsonEscape(occurrence.uri)
                                    << "\",\"range\":{\"start\":{\"line\":" << occurrence.line
                                    << ",\"character\":" << occurrence.startCharacter
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
                             ",\"error\":{\"code\":-32602,\"message\":\"newName must be a valid "
                             "NEXUS identifier\"}}");
                    } else {
                        std::vector<LspIdentifierOccurrence> occurrences;
                        if (document != openDocuments.end()) {
                            const std::string identifier =
                                identifierAtPosition(document->second,
                                                     extractNumber(body, "line"),
                                                     extractNumber(body, "character"));
                            occurrences =
                                findIdentifierOccurrences(identifier,
                                                          uri,
                                                          document->second,
                                                          extractNumber(body, "line"),
                                                          extractNumber(body, "character"));
                        }
                        std::set<std::string> uris;
                        for (const auto& occurrence : occurrences)
                            uris.insert(occurrence.uri);
                        std::ostringstream changes;
                        changes << "{\"changes\":{";
                        bool firstUri = true;
                        for (const auto& changedUri : uris) {
                            if (!firstUri)
                                changes << ',';
                            firstUri = false;
                            changes << "\"" << jsonEscape(changedUri) << "\":[";
                            bool firstEdit = true;
                            for (const auto& occurrence : occurrences) {
                                if (occurrence.uri != changedUri)
                                    continue;
                                if (!firstEdit)
                                    changes << ',';
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
                        send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + changes.str() +
                             "}");
                    }
                } else if (method == "textDocument/definition" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto doc = openDocuments.find(uri);
                    std::string result = "null";
                    const std::size_t requestedLine = extractNumber(body, "line");
                    const std::size_t requestedCharacter = extractNumber(body, "character");
                    if (doc != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(doc->second, requestedLine, requestedCharacter);
                        const auto occurrences = findIdentifierOccurrences(
                            identifier, uri, doc->second, requestedLine, requestedCharacter);
                        for (const auto& occurrence : occurrences) {
                            if (!occurrence.isDeclaration)
                                continue;
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

        bool replStartsWith(const std::string& value, std::string_view prefix) {
            return value.size() >= prefix.size() &&
                   value.compare(0, prefix.size(), prefix.data(), prefix.size()) == 0;
        }

        int replBraceBalance(const std::string& source) {
            int balance = 0;
            bool inString = false;
            bool escaped = false;
            bool inBlockComment = false;

            for (std::size_t i = 0; i < source.size(); ++i) {
                const char ch = source[i];
                const bool hasNext = i + 1 < source.size();
                const char next = hasNext ? source[i + 1] : '\0';

                if (inBlockComment) {
                    if (ch == '*' && next == '/') {
                        inBlockComment = false;
                        ++i;
                    }
                    continue;
                }
                if (inString) {
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        inString = false;
                    }
                    continue;
                }
                if (ch == '/' && next == '/') {
                    while (i < source.size() && source[i] != '\n')
                        ++i;
                    continue;
                }
                if (ch == '/' && next == '*') {
                    inBlockComment = true;
                    ++i;
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                } else if (ch == '{') {
                    ++balance;
                } else if (ch == '}') {
                    --balance;
                }
            }
            return balance;
        }

        bool replHasAssignment(const std::string& source) {
            bool inString = false;
            bool escaped = false;

            for (std::size_t i = 0; i < source.size(); ++i) {
                const char ch = source[i];

                if (inString) {
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        inString = false;
                    }
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                    continue;
                }
                if (ch == '=' && (i + 1 == source.size() || source[i + 1] != '=') &&
                    (i == 0 || std::string("=!<>").find(source[i - 1]) == std::string::npos)) {
                    return i > 0;
                }
            }
            return false;
        }

        bool replIsDeclaration(const std::string& line) {
            return replStartsWith(line, "fn ") || replStartsWith(line, "struct ") ||
                   replStartsWith(line, "enum ") || replStartsWith(line, "extern ") ||
                   replStartsWith(line, "import ");
        }

        bool replIsBlockStatement(const std::string& line) {
            return replStartsWith(line, "if ") || replStartsWith(line, "while ") ||
                   replStartsWith(line, "for ") || replStartsWith(line, "match ") ||
                   replStartsWith(line, "unsafe ");
        }

        bool replExpressionIsReplaySafe(const Expr* expression) {
            if (!expression)
                return true;
            if (dynamic_cast<const IntExpr*>(expression) ||
                dynamic_cast<const FloatExpr*>(expression) ||
                dynamic_cast<const BoolExpr*>(expression) ||
                dynamic_cast<const StringExpr*>(expression) ||
                dynamic_cast<const VarExpr*>(expression) ||
                dynamic_cast<const EnumVariantExpr*>(expression))
                return true;
            if (const auto* array = dynamic_cast<const ArrayExpr*>(expression)) {
                for (const auto& element : array->elements)
                    if (!replExpressionIsReplaySafe(element.get()))
                        return false;
                return true;
            }
            if (const auto* index = dynamic_cast<const IndexExpr*>(expression))
                return replExpressionIsReplaySafe(index->object.get()) &&
                       replExpressionIsReplaySafe(index->index.get());
            if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression))
                return replExpressionIsReplaySafe(unary->rhs.get());
            if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression))
                return replExpressionIsReplaySafe(binary->lhs.get()) &&
                       replExpressionIsReplaySafe(binary->rhs.get());
            if (const auto* member = dynamic_cast<const MemberExpr*>(expression))
                return replExpressionIsReplaySafe(member->object.get());
            if (const auto* initializer = dynamic_cast<const StructInitExpr*>(expression)) {
                for (const auto& field : initializer->fields)
                    if (!replExpressionIsReplaySafe(field.second.get()))
                        return false;
                return true;
            }

            // Calls are intentionally excluded: built-ins and user functions can
            // perform I/O or otherwise change external state when replayed.
            return false;
        }

        bool replStatementIsReplaySafe(const Stmt* statement) {
            if (!statement)
                return false;
            if (const auto* block = dynamic_cast<const Block*>(statement)) {
                for (const auto& nested : block->statements)
                    if (!replStatementIsReplaySafe(nested.get()))
                        return false;
                return true;
            }
            if (const auto* declaration = dynamic_cast<const LetStmt*>(statement))
                return replExpressionIsReplaySafe(declaration->init.get());
            if (const auto* assignment = dynamic_cast<const AssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* assignment = dynamic_cast<const MemberAssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->object.get()) &&
                       replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* assignment = dynamic_cast<const IndexAssignStmt*>(statement))
                return replExpressionIsReplaySafe(assignment->object.get()) &&
                       replExpressionIsReplaySafe(assignment->index.get()) &&
                       replExpressionIsReplaySafe(assignment->value.get());
            if (const auto* expression = dynamic_cast<const ExprStmt*>(statement))
                return replExpressionIsReplaySafe(expression->expr.get());
            if (const auto* branch = dynamic_cast<const IfStmt*>(statement))
                return replExpressionIsReplaySafe(branch->cond.get()) &&
                       replStatementIsReplaySafe(branch->thenBlock.get()) &&
                       (!branch->elseBlock || replStatementIsReplaySafe(branch->elseBlock.get()));
            if (const auto* loop = dynamic_cast<const WhileStmt*>(statement))
                return replExpressionIsReplaySafe(loop->cond.get()) &&
                       replStatementIsReplaySafe(loop->body.get());
            if (const auto* match = dynamic_cast<const MatchStmt*>(statement)) {
                if (!replExpressionIsReplaySafe(match->value.get()))
                    return false;
                for (const auto& arm : match->arms) {
                    if ((!arm.wildcard && !replExpressionIsReplaySafe(arm.pattern.get())) ||
                        !replStatementIsReplaySafe(arm.body.get()))
                        return false;
                }
                return true;
            }
            if (dynamic_cast<const BreakStmt*>(statement) ||
                dynamic_cast<const ContinueStmt*>(statement))
                return true;

            // Return statements and unknown statement kinds can change the control
            // flow of later replayed entries, so do not persist them.
            return false;
        }

        bool replIsReplaySafeBlock(const std::string& line) {
            try {
                const std::string wrapper = "fn __nexus_repl__() {\n" + line + "\n}\n";
                Program program = Parser(Lexer(wrapper).tokenize()).parseProgram();
                if (program.functions.size() != 1 || !program.functions.front().body ||
                    program.functions.front().body->statements.size() != 1)
                    return false;
                return replStatementIsReplaySafe(
                    program.functions.front().body->statements.front().get());
            } catch (const std::exception&) {
                return false;
            }
        }

        bool replIsStoredStatement(const std::string& line) {
            return replStartsWith(line, "let ") || replStartsWith(line, "const ") ||
                   replHasAssignment(line);
        }

        bool replIsDirectStatement(const std::string& line) {
            return replStartsWith(line, "print(") || replStartsWith(line, "print (") ||
                   replIsBlockStatement(line);
        }

        bool replBareFunctionCall(const std::string& source, std::string& functionName) {
            std::string line = trim(source);
            if (!line.empty() && line.back() == ';') {
                line.pop_back();
                line = trim(line);
            }
            if (line.empty() ||
                !(std::isalpha(static_cast<unsigned char>(line.front())) ||
                  line.front() == '_'))
                return false;

            std::size_t nameEnd = 1;
            while (nameEnd < line.size()) {
                const unsigned char ch = static_cast<unsigned char>(line[nameEnd]);
                if (!std::isalnum(ch) && ch != '_')
                    break;
                ++nameEnd;
            }
            functionName = line.substr(0, nameEnd);
            std::size_t open = nameEnd;
            while (open < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[open])))
                ++open;
            if (open >= line.size() || line[open] != '(')
                return false;

            std::size_t depth = 0;
            bool inString = false;
            bool escaped = false;
            for (std::size_t i = open; i < line.size(); ++i) {
                const char ch = line[i];
                const bool hasNext = i + 1 < line.size();
                const char next = hasNext ? line[i + 1] : '\0';
                if (inString) {
                    if (escaped)
                        escaped = false;
                    else if (ch == '\\')
                        escaped = true;
                    else if (ch == '"')
                        inString = false;
                    continue;
                }
                if (ch == '/' && next == '/')
                    return trim(line.substr(i)).empty();
                if (ch == '/' && next == '*') {
                    const auto close = line.find("*/", i + 2);
                    if (close == std::string::npos)
                        return false;
                    i = close + 1;
                    continue;
                }
                if (ch == '"') {
                    inString = true;
                } else if (ch == '(') {
                    ++depth;
                } else if (ch == ')') {
                    if (depth == 0)
                        return false;
                    --depth;
                    if (depth == 0)
                        return trim(line.substr(i + 1)).empty();
                }
            }
            return false;
        }

        bool replIsVoidFunctionCall(const std::string& line,
                                    const std::vector<std::string>& declarations) {
            std::string calledName;
            if (!replBareFunctionCall(line, calledName))
                return false;

            for (const auto& declaration : declarations) {
                const std::string header = trim(declaration);
                if (!replStartsWith(header, "fn "))
                    continue;
                std::size_t nameStart = 3;
                while (nameStart < header.size() &&
                       std::isspace(static_cast<unsigned char>(header[nameStart])))
                    ++nameStart;
                std::size_t nameEnd = nameStart;
                while (nameEnd < header.size()) {
                    const unsigned char ch = static_cast<unsigned char>(header[nameEnd]);
                    if (!std::isalnum(ch) && ch != '_')
                        break;
                    ++nameEnd;
                }
                if (header.substr(nameStart, nameEnd - nameStart) != calledName)
                    continue;
                const auto bodyStart = header.find('{', nameEnd);
                const auto declarationEnd =
                    bodyStart == std::string::npos ? header.find(';', nameEnd) : bodyStart;
                const std::string signature = header.substr(
                    nameEnd,
                    declarationEnd == std::string::npos ? std::string::npos
                                                        : declarationEnd - nameEnd);
                return signature.find("->") == std::string::npos;
            }
            return false;
        }

        std::string replBindingName(const std::string& line) {
            std::string rest;
            if (replStartsWith(line, "let ")) {
                rest = trim(line.substr(4));
                if (replStartsWith(rest, "mut "))
                    rest = trim(rest.substr(4));
            } else if (replStartsWith(line, "const ")) {
                rest = trim(line.substr(6));
            } else {
                return {};
            }

            std::size_t end = 0;
            while (end < rest.size()) {
                const unsigned char ch = static_cast<unsigned char>(rest[end]);
                if (!std::isalnum(ch) && rest[end] != '_')
                    break;
                ++end;
            }
            return rest.substr(0, end);
        }

        void replWriteIndented(std::ostream& output, const std::string& source) {
            std::istringstream input(source);
            std::string line;
            while (std::getline(input, line)) {
                output << "    " << line << '\n';
            }
        }

        constexpr std::size_t kReplHistoryLimit = 500;

        fs::path replHistoryPath() {
            if (const char* overridePath = std::getenv("NEXUS_HISTORY_FILE")) {
                if (*overridePath != '\0')
                    return fs::path(overridePath);
            }
#ifdef _WIN32
            if (const char* appData = std::getenv("APPDATA")) {
                if (*appData != '\0')
                    return fs::path(appData) / "NEXUS" / "repl_history.txt";
            }
            if (const char* home = std::getenv("USERPROFILE")) {
                if (*home != '\0')
                    return fs::path(home) / ".nexus_repl_history";
            }
#else
            if (const char* home = std::getenv("HOME")) {
                if (*home != '\0')
                    return fs::path(home) / ".nexus_repl_history";
            }
#endif
            return {};
        }

        std::string encodeReplHistoryEntry(const std::string& entry) {
            std::string out;
            for (char ch : entry) {
                switch (ch) {
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                default:
                    out += ch;
                    break;
                }
            }
            return out;
        }

        std::string decodeReplHistoryEntry(const std::string& encoded) {
            std::string out;
            for (std::size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] != '\\' || i + 1 >= encoded.size()) {
                    out += encoded[i];
                    continue;
                }
                const char ch = encoded[++i];
                if (ch == 'n') {
                    out += '\n';
                } else if (ch == 'r') {
                    out += '\r';
                } else if (ch == '\\') {
                    out += '\\';
                } else {
                    out += '\\';
                    out += ch;
                }
            }
            return out;
        }

        std::vector<std::string> loadReplHistory(const fs::path& path) {
            std::vector<std::string> entries;
            if (path.empty())
                return entries;

            std::ifstream in(path, std::ios::binary);
            std::string line;
            while (std::getline(in, line)) {
                entries.push_back(decodeReplHistoryEntry(line));
                if (entries.size() > kReplHistoryLimit)
                    entries.erase(entries.begin());
            }
            return entries;
        }

        bool saveReplHistory(const fs::path& path, const std::vector<std::string>& entries) {
            if (path.empty())
                return true;

            try {
                std::ostringstream serialized;
                for (const auto& entry : entries)
                    serialized << encodeReplHistoryEntry(entry) << '\n';

                // Keep the last valid history file if the process is interrupted
                // while writing the next version.
                writeTextAtomically(path, serialized.str());
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }

        int commandRepl() {
            std::cout << "NEXUS " << kVersion << " interactive shell\n"
                      << "Enter an expression to evaluate it. Use let x = 10 to keep a value.\n"
                      << "Commands: :help, :vars, :history, :history-clear, :clear, :quit\n\n";

            const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path tempDir =
                fs::temp_directory_path() / ("nexus-repl-" + std::to_string(tick));
            fs::create_directories(tempDir);

            struct TempDirectoryCleanup {
                fs::path path;
                ~TempDirectoryCleanup() {
                    std::error_code ec;
                    fs::remove_all(path, ec);
                }
            } cleanup{tempDir};

            const fs::path sourcePath = tempDir / "repl.nx";
#ifdef _WIN32
            const fs::path outputPath = tempDir / "repl.exe";
#else
            const fs::path outputPath = tempDir / "repl";
#endif
            std::vector<std::string> declarations;
            std::vector<std::string> statements;
            std::vector<std::string> variableNames;
            const fs::path persistentHistoryPath = replHistoryPath();
            std::vector<std::string> history = loadReplHistory(persistentHistoryPath);
            bool historySaveWarningShown = false;
            const auto persistHistory = [&]() {
                if (!saveReplHistory(persistentHistoryPath, history) &&
                    !historySaveWarningShown) {
                    std::cerr << "nexus: warning: could not save REPL history to "
                              << persistentHistoryPath << '\n';
                    historySaveWarningShown = true;
                }
            };

            using ReplValue = std::variant<long long, double, bool, std::string>;
            struct ReplBinding {
                ReplValue value;
                bool mutableValue = false;
            };
            std::unordered_map<std::string, ReplBinding> variables;
            bool fastPathEnabled = true;

            enum class ReplMode { None, Expression, Statement };
            enum class FastLineResult { Handled, Unsupported, Error };

            const auto evalExpression = [&](const auto& self,
                                            const Expr* expression,
                                            ReplValue& result,
                                            std::string& error) -> bool {
                if (!expression)
                    return false;
                if (const auto* literal = dynamic_cast<const IntExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* literal = dynamic_cast<const FloatExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* literal = dynamic_cast<const BoolExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* literal = dynamic_cast<const StringExpr*>(expression)) {
                    result = literal->value;
                    return true;
                }
                if (const auto* variable = dynamic_cast<const VarExpr*>(expression)) {
                    const auto found = variables.find(variable->name);
                    if (found == variables.end())
                        return false;
                    result = found->second.value;
                    return true;
                }
                if (const auto* unary = dynamic_cast<const UnaryExpr*>(expression)) {
                    ReplValue operand;
                    if (!self(self, unary->rhs.get(), operand, error))
                        return false;
                    if (unary->op == TokenKind::Bang) {
                        if (const auto* value = std::get_if<bool>(&operand)) {
                            result = !*value;
                            return true;
                        }
                    } else if (unary->op == TokenKind::Minus) {
                        if (const auto* value = std::get_if<long long>(&operand)) {
                            if (*value == std::numeric_limits<long long>::min())
                                return false;
                            result = -*value;
                            return true;
                        }
                        if (const auto* value = std::get_if<double>(&operand)) {
                            result = -*value;
                            return true;
                        }
                    } else if (unary->op == TokenKind::Plus) {
                        if (std::holds_alternative<long long>(operand) ||
                            std::holds_alternative<double>(operand)) {
                            result = operand;
                            return true;
                        }
                    }
                    return false;
                }
                if (const auto* binary = dynamic_cast<const BinaryExpr*>(expression)) {
                    ReplValue lhs;
                    ReplValue rhs;
                    if (!self(self, binary->lhs.get(), lhs, error) ||
                        !self(self, binary->rhs.get(), rhs, error))
                        return false;

                    if (binary->op == TokenKind::AndAnd || binary->op == TokenKind::OrOr) {
                        const auto* left = std::get_if<bool>(&lhs);
                        const auto* right = std::get_if<bool>(&rhs);
                        if (!left || !right)
                            return false;
                        result =
                            binary->op == TokenKind::AndAnd ? (*left && *right) : (*left || *right);
                        return true;
                    }

                    const bool lhsInteger = std::holds_alternative<long long>(lhs);
                    const bool rhsInteger = std::holds_alternative<long long>(rhs);
                    const bool lhsNumber = lhsInteger || std::holds_alternative<double>(lhs);
                    const bool rhsNumber = rhsInteger || std::holds_alternative<double>(rhs);

                    if (binary->op == TokenKind::Plus && std::holds_alternative<std::string>(lhs) &&
                        std::holds_alternative<std::string>(rhs)) {
                        result = std::get<std::string>(lhs) + std::get<std::string>(rhs);
                        return true;
                    }

                    if (lhsNumber && rhsNumber) {
                        if (binary->op == TokenKind::EqualEqual ||
                            binary->op == TokenKind::BangEqual || binary->op == TokenKind::Less ||
                            binary->op == TokenKind::LessEqual ||
                            binary->op == TokenKind::Greater ||
                            binary->op == TokenKind::GreaterEqual) {
                            bool comparison = false;
                            if (lhsInteger && rhsInteger) {
                                const auto left = std::get<long long>(lhs);
                                const auto right = std::get<long long>(rhs);
                                switch (binary->op) {
                                case TokenKind::EqualEqual:
                                    comparison = left == right;
                                    break;
                                case TokenKind::BangEqual:
                                    comparison = left != right;
                                    break;
                                case TokenKind::Less:
                                    comparison = left < right;
                                    break;
                                case TokenKind::LessEqual:
                                    comparison = left <= right;
                                    break;
                                case TokenKind::Greater:
                                    comparison = left > right;
                                    break;
                                case TokenKind::GreaterEqual:
                                    comparison = left >= right;
                                    break;
                                default:
                                    return false;
                                }
                            } else {
                                const double left =
                                    lhsInteger ? static_cast<double>(std::get<long long>(lhs))
                                               : std::get<double>(lhs);
                                const double right =
                                    rhsInteger ? static_cast<double>(std::get<long long>(rhs))
                                               : std::get<double>(rhs);
                                switch (binary->op) {
                                case TokenKind::EqualEqual:
                                    comparison = left == right;
                                    break;
                                case TokenKind::BangEqual:
                                    comparison = left != right;
                                    break;
                                case TokenKind::Less:
                                    comparison = left < right;
                                    break;
                                case TokenKind::LessEqual:
                                    comparison = left <= right;
                                    break;
                                case TokenKind::Greater:
                                    comparison = left > right;
                                    break;
                                case TokenKind::GreaterEqual:
                                    comparison = left >= right;
                                    break;
                                default:
                                    return false;
                                }
                            }
                            result = comparison;
                            return true;
                        }

                        if (lhsInteger && rhsInteger) {
                            const long long left = std::get<long long>(lhs);
                            const long long right = std::get<long long>(rhs);
                            if (binary->op == TokenKind::Slash ||
                                binary->op == TokenKind::Percent) {
                                if (right == 0) {
                                    error = "division by zero";
                                    return false;
                                }
                                if (left == std::numeric_limits<long long>::min() && right == -1)
                                    return false;
                                result = binary->op == TokenKind::Slash ? ReplValue(left / right)
                                                                        : ReplValue(left % right);
                                return true;
                            }
                            long long calculated = 0;
                            const auto minimum = std::numeric_limits<long long>::min();
                            const auto maximum = std::numeric_limits<long long>::max();
                            if (binary->op == TokenKind::Plus) {
                                if ((right > 0 && left > maximum - right) ||
                                    (right < 0 && left < minimum - right))
                                    return false;
                                calculated = left + right;
                            } else if (binary->op == TokenKind::Minus) {
                                if ((right < 0 && left > maximum + right) ||
                                    (right > 0 && left < minimum + right))
                                    return false;
                                calculated = left - right;
                            } else if (binary->op == TokenKind::Star) {
                                if (left != 0 && right != 0) {
                                    if ((left == -1 && right == minimum) ||
                                        (right == -1 && left == minimum))
                                        return false;
                                    if (left > 0) {
                                        if ((right > 0 && left > maximum / right) ||
                                            (right < 0 && right < minimum / left))
                                            return false;
                                    } else {
                                        if ((right > 0 && left < minimum / right) ||
                                            (right < 0 && left < maximum / right))
                                            return false;
                                    }
                                }
                                calculated = left * right;
                            } else {
                                return false;
                            }
                            result = calculated;
                            return true;
                        }

                        const double left = lhsInteger
                                                ? static_cast<double>(std::get<long long>(lhs))
                                                : std::get<double>(lhs);
                        const double right = rhsInteger
                                                 ? static_cast<double>(std::get<long long>(rhs))
                                                 : std::get<double>(rhs);
                        if (binary->op == TokenKind::Slash || binary->op == TokenKind::Percent) {
                            if (right == 0.0) {
                                error = "division by zero";
                                return false;
                            }
                            if (binary->op == TokenKind::Percent)
                                result = std::fmod(left, right);
                            else
                                result = left / right;
                            return true;
                        }
                        if (binary->op == TokenKind::Plus)
                            result = left + right;
                        else if (binary->op == TokenKind::Minus)
                            result = left - right;
                        else if (binary->op == TokenKind::Star)
                            result = left * right;
                        else
                            return false;
                        return true;
                    }

                    if (binary->op == TokenKind::EqualEqual || binary->op == TokenKind::BangEqual) {
                        if (lhs.index() != rhs.index())
                            return false;
                        const bool equal = lhs == rhs;
                        result = binary->op == TokenKind::EqualEqual ? equal : !equal;
                        return true;
                    }
                }
                return false;
            };

            const auto tryFastLine =
                [&](const std::string& line, bool stored, bool expression) -> FastLineResult {
                try {
                    const std::string wrapper = "fn __nexus_repl__() {\n" + line + "\n}\n";
                    Program program = Parser(Lexer(wrapper).tokenize()).parseProgram();
                    if (program.functions.size() != 1 || !program.functions.front().body ||
                        program.functions.front().body->statements.size() != 1)
                        return FastLineResult::Unsupported;

                    auto& statement = program.functions.front().body->statements.front();
                    if (stored) {
                        if (auto* declaration = dynamic_cast<LetStmt*>(statement.get())) {
                            if (!declaration->init ||
                                variables.find(declaration->name) != variables.end())
                                return FastLineResult::Unsupported;
                            ReplValue value;
                            std::string error;
                            if (!evalExpression(
                                    evalExpression, declaration->init.get(), value, error)) {
                                if (!error.empty()) {
                                    std::cerr << "nexus: " << error << '\n';
                                    return FastLineResult::Error;
                                }
                                return FastLineResult::Unsupported;
                            }
                            const bool compatible = declaration->type.kind == TypeKind::Unknown ||
                                                    (declaration->type.kind == TypeKind::I64 &&
                                                     std::holds_alternative<long long>(value)) ||
                                                    (declaration->type.kind == TypeKind::F64 &&
                                                     std::holds_alternative<double>(value)) ||
                                                    (declaration->type.kind == TypeKind::Bool &&
                                                     std::holds_alternative<bool>(value)) ||
                                                    (declaration->type.kind == TypeKind::String &&
                                                     std::holds_alternative<std::string>(value));
                            if (!compatible)
                                return FastLineResult::Unsupported;
                            variables.emplace(declaration->name,
                                              ReplBinding{std::move(value), declaration->mut});
                            return FastLineResult::Handled;
                        }
                        if (auto* assignment = dynamic_cast<AssignStmt*>(statement.get())) {
                            const auto found = variables.find(assignment->name);
                            if (found == variables.end() || !found->second.mutableValue)
                                return FastLineResult::Unsupported;
                            ReplValue value;
                            std::string error;
                            if (!evalExpression(
                                    evalExpression, assignment->value.get(), value, error)) {
                                if (!error.empty()) {
                                    std::cerr << "nexus: " << error << '\n';
                                    return FastLineResult::Error;
                                }
                                return FastLineResult::Unsupported;
                            }
                            if (found->second.value.index() != value.index())
                                return FastLineResult::Unsupported;
                            found->second.value = std::move(value);
                            return FastLineResult::Handled;
                        }
                        return FastLineResult::Unsupported;
                    }
                    if (expression) {
                        const auto* expressionStatement =
                            dynamic_cast<const ExprStmt*>(statement.get());
                        if (!expressionStatement || !expressionStatement->expr)
                            return FastLineResult::Unsupported;
                        ReplValue value;
                        std::string error;
                        if (!evalExpression(
                                evalExpression, expressionStatement->expr.get(), value, error)) {
                            if (!error.empty()) {
                                std::cerr << "nexus: " << error << '\n';
                                return FastLineResult::Error;
                            }
                            return FastLineResult::Unsupported;
                        }
                        if (const auto* integer = std::get_if<long long>(&value)) {
                            std::cout << *integer << '\n';
                        } else if (const auto* floating = std::get_if<double>(&value)) {
                            std::cout << std::setprecision(15) << *floating << '\n';
                        } else if (const auto* boolean = std::get_if<bool>(&value)) {
                            std::cout << (*boolean ? "true" : "false") << '\n';
                        } else {
                            std::cout << std::get<std::string>(value) << '\n';
                        }
                        return FastLineResult::Handled;
                    }
                } catch (const std::exception&) {
                    return FastLineResult::Unsupported;
                }
                return FastLineResult::Unsupported;
            };

            const auto execute = [&](const std::vector<std::string>& sourceDeclarations,
                                     const std::vector<std::string>& sourceStatements,
                                     const std::string& currentLine,
                                     ReplMode mode) -> bool {
                std::ofstream source(sourcePath, std::ios::binary | std::ios::trunc);
                if (!source) {
                    std::cerr << "nexus: cannot write temporary REPL source\n";
                    return false;
                }

                for (const auto& declaration : sourceDeclarations) {
                    source << declaration << '\n';
                }
                source << "\nfn main() {\n";
                for (const auto& statement : sourceStatements) {
                    replWriteIndented(source, statement);
                }
                if (mode == ReplMode::Expression) {
                    source << "    print(" << currentLine << ")\n";
                } else if (mode == ReplMode::Statement) {
                    replWriteIndented(source, currentLine);
                }
                source << "}\n";
                source.close();

                std::error_code ec;
                fs::remove(outputPath, ec);
                fs::path llvmIrPath = outputPath;
                llvmIrPath += ".ll";
                fs::remove(llvmIrPath, ec);

                try {
                    const int buildResult =
                        compileFile(sourcePath, outputPath, "native", false, 0, "auto", true);
                    if (buildResult != 0)
                        return false;

                    const int runResult = launchExecutable(outputPath, tempDir);
                    if (runResult != 0) {
                        std::cerr << "nexus: expression exited with status " << runResult << '\n';
                        return false;
                    }
                    return true;
                } catch (const std::exception& error) {
                    std::cerr << formatDiagnostic(error) << '\n';
                    return false;
                }
            };

            std::string pending;
            std::string inputLine;
            for (;;) {
                std::cout << (pending.empty() ? "nexus> " : "   ... ") << std::flush;
                if (!std::getline(std::cin, inputLine)) {
                    if (!pending.empty()) {
                        std::cerr << "nexus: incomplete multi-line input discarded\n";
                    }
                    break;
                }

                if (pending.empty()) {
                    inputLine = trim(inputLine);
                    if (inputLine.empty())
                        continue;

                    if (inputLine == ":quit" || inputLine == ":q" || inputLine == "quit" ||
                        inputLine == "quit()" || inputLine == "exit" || inputLine == "exit()") {
                        break;
                    }
                    if (inputLine == ":help") {
                        std::cout << "Expressions: 2 + 2, (8 + 4) / 3, 2.5 * 4\n"
                                  << "Bindings:    let answer = 42\n"
                                  << "Assignment:  answer = answer + 1\n"
                                  << "Functions:   fn double(x: i64) -> i64 { return x * 2 }\n"
                                  << "State is rebuilt from saved declarations and statements for "
                                     "each entry.\n"
                                  << ":vars lists bindings; :history shows this session's input; "
                                     ":history-clear clears it.\n"
                                  << ":clear resets code and bindings; :quit exits.\n";
                        continue;
                    }
                    if (inputLine == ":clear") {
                        declarations.clear();
                        statements.clear();
                        variableNames.clear();
                        variables.clear();
                        fastPathEnabled = true;
                        std::cout << "session cleared\n";
                        continue;
                    }
                    if (inputLine == ":vars") {
                        if (variableNames.empty()) {
                            std::cout << "no saved bindings\n";
                        } else {
                            for (const auto& name : variableNames)
                                std::cout << name << '\n';
                        }
                        continue;
                    }
                    if (inputLine == ":history") {
                        if (history.empty()) {
                            std::cout << "history is empty\n";
                        } else {
                            for (std::size_t i = 0; i < history.size(); ++i)
                                std::cout << (i + 1) << "  " << history[i] << '\n';
                        }
                        continue;
                    }
                    if (inputLine == ":history-clear") {
                        history.clear();
                        persistHistory();
                        std::cout << "history cleared\n";
                        continue;
                    }

                    if ((replIsDeclaration(inputLine) || replIsBlockStatement(inputLine)) &&
                        replBraceBalance(inputLine) > 0) {
                        pending = inputLine;
                        continue;
                    }
                } else {
                    pending += '\n';
                    pending += inputLine;
                    if (replBraceBalance(pending) > 0)
                        continue;
                    inputLine = trim(pending);
                    pending.clear();
                }

                history.push_back(inputLine);
                if (history.size() > kReplHistoryLimit) {
                    const auto excess = static_cast<std::vector<std::string>::difference_type>(
                        history.size() - kReplHistoryLimit);
                    history.erase(history.begin(), history.begin() + excess);
                }
                persistHistory();

                if (replIsDeclaration(inputLine)) {
                    auto candidate = declarations;
                    candidate.push_back(inputLine);
                    if (execute(candidate, statements, {}, ReplMode::None)) {
                        declarations = std::move(candidate);
                    }
                    continue;
                }

                if (replIsStoredStatement(inputLine)) {
                    const auto fastResult = fastPathEnabled ? tryFastLine(inputLine, true, false)
                                                            : FastLineResult::Unsupported;
                    if (fastResult == FastLineResult::Handled) {
                        statements.push_back(inputLine);
                        const std::string name = replBindingName(inputLine);
                        if (!name.empty() &&
                            std::find(variableNames.begin(), variableNames.end(), name) ==
                                variableNames.end()) {
                            variableNames.push_back(name);
                        }
                        continue;
                    }
                    if (fastResult == FastLineResult::Error)
                        continue;

                    auto candidate = statements;
                    candidate.push_back(inputLine);
                    if (execute(declarations, candidate, {}, ReplMode::None)) {
                        statements = std::move(candidate);
                        fastPathEnabled = false;
                        const std::string name = replBindingName(inputLine);
                        if (!name.empty() &&
                            std::find(variableNames.begin(), variableNames.end(), name) ==
                                variableNames.end()) {
                            variableNames.push_back(name);
                        }
                    }
                    continue;
                }

                if (replIsDirectStatement(inputLine) ||
                    replIsVoidFunctionCall(inputLine, declarations)) {
                    const bool isControlBlock = replIsBlockStatement(inputLine);
                    const bool replaySafeBlock =
                        isControlBlock && replIsReplaySafeBlock(inputLine);
                    if (execute(declarations, statements, inputLine, ReplMode::Statement) &&
                        isControlBlock) {
                        fastPathEnabled = false;
                        if (replaySafeBlock) {
                            // Pure state-changing blocks must be replayed after
                            // declarations/assignments so later entries see the same state.
                            statements.push_back(inputLine);
                        } else {
                            std::cerr
                                << "nexus: note: this control-flow block contains calls or "
                                   "unsupported operations, so it is not replayed in later entries; "
                                   "variable changes made inside it will not persist.\n";
                        }
                    }
                    continue;
                }

                if (!inputLine.empty() && inputLine.back() == ';') {
                    inputLine.pop_back();
                    inputLine = trim(inputLine);
                }
                if (inputLine.empty())
                    continue;
                if (fastPathEnabled) {
                    const auto fastResult = tryFastLine(inputLine, false, true);
                    if (fastResult == FastLineResult::Handled ||
                        fastResult == FastLineResult::Error) {
                        continue;
                    }
                }
                execute(declarations, statements, inputLine, ReplMode::Expression);
            }

            std::cout << '\n';
            return 0;
        }

    } // namespace

    int run(int argc, char** argv) {

        try {

            gExecutable = argc > 0 ? fs::absolute(argv[0]) : fs::path();

            if (argc < 2)
                return commandRepl();
            const std::string firstArg = argv[1];
            std::string firstExtension = fs::path(firstArg).extension().string();
            std::transform(firstExtension.begin(),
                           firstExtension.end(),
                           firstExtension.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            const bool directFileInvocation = (firstExtension == ".nx");
            const std::string cmd = directFileInvocation ? "run" : firstArg;

            if (cmd == "repl")
                return commandRepl();

            if (cmd == "help" || cmd == "--help") {
                usage();
                return 0;
            }
            if (cmd == "version" || cmd == "--version") {

                std::cout << "Nexus " << kVersion
                          << " (C++20/LLVM IR + Native Graphics/Audio/Game Runtime)\n";

                return 0;
            }
            if (cmd == "targets") {

                std::cout
                    << "native\nx86_64-linux\naarch64-linux\nx86_64-windows\naarch64-windows\n"
                       "x86_64-macos\naarch64-macos\naarch64-android\nwasm32-wasi\n"
                       "\nbuiltin native backend (no external toolchain):\n"
                       "  x86_64-windows: PE32+ console executable\n"
                       "  x86_64-linux:   ELF64 executable\n"
                       "  native:         host default (x86-64 hosts only)\n";

                return 0;
            }

            if (cmd == "new") {

                if (argc < 3)
                    throw std::runtime_error("new requires a project name");

                std::string templ = "cli";

                for (int i = 3; i < argc; ++i) {

                    const std::string a = argv[i];

                    if (a == "--template" && i + 1 < argc)
                        templ = argv[++i];

                    else if (a.rfind("--template=", 0) == 0)
                        templ = a.substr(11);

                    else
                        throw std::runtime_error("unknown option: " + a);
                }
                if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" &&
                    templ != "paint" && templ != "server" && templ != "library")
                    throw std::runtime_error("unknown template: " + templ);

                const fs::path projectPath = fs::absolute(argv[2]);

                return initProject(projectPath, projectPath.filename().string(), templ, true);
            }

            if (cmd == "init") {

                std::string templ = "cli";

                for (int i = 2; i < argc; ++i) {

                    const std::string a = argv[i];

                    if (a == "--template" && i + 1 < argc)
                        templ = argv[++i];

                    else if (a.rfind("--template=", 0) == 0)
                        templ = a.substr(11);

                    else
                        throw std::runtime_error("unknown option: " + a);
                }
                if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" &&
                    templ != "paint" && templ != "server" && templ != "library")
                    throw std::runtime_error("unknown template: " + templ);

                const fs::path root = fs::current_path();

                return initProject(root, root.filename().string(), templ, false);
            }

            if (cmd == "info")
                return commandInfo();

            if (cmd == "assets")
                return commandAssets();

            if (cmd == "clean")
                return commandClean();

            if (cmd == "doctor") {
                bool json = false;
                for (int i = 2; i < argc; ++i) {
                    const std::string option = argv[i];
                    if (option == "--json")
                        json = true;
                    else
                        throw std::runtime_error("unknown doctor option: " + option);
                }
                return commandDoctor(json);
            }

            if (cmd == "self-test")
                return commandSelfTest();

            if (cmd == "explain-backend") {
                if (argc < 3)
                    throw std::runtime_error("explain-backend requires a source file (usage: nexus "
                                             "explain-backend file.nx [--target TRIPLE])");
                std::string target = "native";
                for (int i = 3; i < argc; ++i) {
                    const std::string option = argv[i];
                    if (option == "--target" && i + 1 < argc)
                        target = argv[++i];
                    else if (option.rfind("--target=", 0) == 0)
                        target = option.substr(9);
                    else
                        throw std::runtime_error("unknown explain-backend option: " + option);
                }
                return commandExplainBackend(resolveInput(argv[2]), target);
            }

            if (cmd == "fmt" || cmd == "format") {
                bool checkOnly = false;
                bool allFiles = false;
                std::string input;
                for (int i = 2; i < argc; ++i) {
                    const std::string option = argv[i];
                    if (option == "--check")
                        checkOnly = true;
                    else if (option == "--all")
                        allFiles = true;
                    else if (!option.empty() && option.front() == '-')
                        throw std::runtime_error("unknown fmt option: " + option);
                    else if (input.empty())
                        input = option;
                    else
                        throw std::runtime_error("fmt accepts only one source file");
                }
                if (allFiles) {
                    if (!input.empty())
                        throw std::runtime_error("fmt --all does not accept a source-file argument");
                    return commandFmtAll(checkOnly);
                }
                return commandFmt(resolveInput(input), checkOnly);
            }
            if (cmd == "lint") {

                const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

                return commandLint(in);
            }
            if (cmd == "doc")
                return commandDoc();

            if (cmd == "bench") {

                const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

                int runs = 5;
                for (int i = 2; i < argc; ++i) {
                    std::string a = argv[i];
                    if (a == "--runs" && i + 1 < argc)
                        runs = std::stoi(argv[++i]);
                    else if (a.rfind("--runs=", 0) == 0)
                        runs = std::stoi(a.substr(7));
                }
                return commandBench(in, runs);
            }
            if (cmd == "test")
                return commandTest();

            if (cmd == "export") {
                std::vector<std::string> args;
                for (int i = 2; i < argc; ++i)
                    args.emplace_back(argv[i]);
                return commandExport(args);
            }

            if (cmd == "editor") {

                bool gui = false;
                for (int i = 2; i < argc; ++i)
                    if (std::string(argv[i]) == "--gui")
                        gui = true;

                if (!gui)
                    return commandEditor();

                const auto tpl = templateSource("editor");
                if (tpl.empty())
                    throw std::runtime_error("built-in editor template not found");

                const auto out = fs::temp_directory_path() / "nexus-editor";
                compileFile(tpl, out, "native", false, 2);

                return runCommand({out.string()});
            }
            if (cmd == "lsp")
                return commandLsp();

            if (cmd == "pkg") {
                std::vector<std::string> args;
                for (int i = 2; i < argc; ++i)
                    args.emplace_back(argv[i]);
                return commandPkg(args);
            }

            if (cmd == "check") {

                const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");

                ModuleLoadContext loadContext;

                auto prog = loadProgram(in, loadContext);

                SemanticAnalyzer().analyze(prog);

                std::cout << "check: OK\n";

                return 0;
            }

            if (cmd == "build" || cmd == "run") {

                int argi = directFileInvocation ? 1 : 2;

                std::string input;

                if (argi < argc && argv[argi][0] != '-')
                    input = argv[argi++];

                ProjectConfig cfg;

                bool hasProject = false;

                try {
                    cfg = loadProject();
                    hasProject = true;
                } catch (...) {}
                const auto in = resolveInput(input);

                std::string target = hasProject && input.empty() ? cfg.target : "native";

                std::string explicitOutput;

                bool emitIR = false;

                bool release = false;

                std::string backend = "auto";

                for (int i = argi; i < argc; ++i) {

                    const std::string a = argv[i];

                    if (a == "--emit-ir")
                        emitIR = true;

                    else if (a == "--release")
                        release = true;

                    else if (a == "-o" && i + 1 < argc)
                        explicitOutput = argv[++i];

                    else if (a.rfind("--target=", 0) == 0)
                        target = a.substr(9);

                    else if (a == "--target" && i + 1 < argc)
                        target = argv[++i];

                    else if (a.rfind("--backend=", 0) == 0)
                        backend = a.substr(10);

                    else if (a == "--backend" && i + 1 < argc)
                        backend = argv[++i];

                    else
                        throw std::runtime_error("unknown option: " + a);
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
                    const fs::path root =
                        hasProject && input.empty() ? cfg.root : out.parent_path();
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
