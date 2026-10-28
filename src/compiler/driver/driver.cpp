#include "driver/driver_internal.hpp"

namespace nexus {

    int run(int argc, char** argv) {
        using namespace driver;

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

            if (cmd == "info") {
                if (argc != 2)
                    throw std::runtime_error("info does not accept arguments");
                return commandInfo();
            }

            if (cmd == "assets") {
                if (argc != 2)
                    throw std::runtime_error("assets does not accept arguments");
                return commandAssets();
            }

            if (cmd == "clean") {
                if (argc != 2)
                    throw std::runtime_error("clean does not accept arguments");
                return commandClean();
            }

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
                        throw std::runtime_error(
                            "fmt --all does not accept a source-file argument");
                    return commandFmtAll(checkOnly);
                }
                return commandFmt(resolveInput(input), checkOnly);
            }
            if (cmd == "lint") {
                std::string input;
                for (int i = 2; i < argc; ++i) {
                    const std::string option = argv[i];
                    if (!option.empty() && option.front() == '-')
                        throw std::runtime_error("unknown lint option: " + option);
                    if (!input.empty())
                        throw std::runtime_error("lint accepts only one source file");
                    input = option;
                }
                return commandLint(resolveInput(input));
            }
            if (cmd == "doc") {
                if (argc != 2)
                    throw std::runtime_error("doc does not accept arguments");
                return commandDoc();
            }

            if (cmd == "bench") {
                std::string input;
                int runs = 5;
                const auto parsePositiveRuns = [](const std::string& value) {
                    int parsed = 0;
                    std::size_t consumed = 0;
                    try {
                        parsed = std::stoi(value, &consumed);
                    } catch (const std::exception&) {
                        throw std::runtime_error("bench --runs expects a positive integer");
                    }
                    if (consumed != value.size() || parsed <= 0)
                        throw std::runtime_error("bench --runs expects a positive integer");
                    return parsed;
                };

                for (int i = 2; i < argc; ++i) {
                    const std::string option = argv[i];
                    if (option == "--runs") {
                        if (i + 1 >= argc)
                            throw std::runtime_error("bench --runs requires a value");
                        runs = parsePositiveRuns(argv[++i]);
                    } else if (option.rfind("--runs=", 0) == 0) {
                        runs = parsePositiveRuns(option.substr(7));
                    } else if (!option.empty() && option.front() == '-') {
                        throw std::runtime_error("unknown bench option: " + option);
                    } else if (input.empty()) {
                        input = option;
                    } else {
                        throw std::runtime_error("bench accepts only one source file");
                    }
                }
                return commandBench(resolveInput(input), runs);
            }
            if (cmd == "test") {
                if (argc != 2)
                    throw std::runtime_error("test does not accept arguments");
                return commandTest();
            }

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

                if (rc == 0 && windowsTarget)
                    deployRuntimeLibraries(out);

                if (cmd == "run" && rc == 0) {
                    // An explicit source file runs from the directory the command
                    // was invoked in; `nexus run` without a file runs from the
                    // project root so relative assets keep resolving there.
                    const fs::path root =
                        input.empty() ? (hasProject ? cfg.root : fs::current_path())
                                      : fs::current_path();
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
