#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


        // Launch child processes directly instead of routing through a shell.
        // The previous implementation built a single command string and passed
        // it to std::system(), which breaks for quoted program paths containing
        // spaces (cmd.exe strips the outer quotes) and needlessly depends on a
        // command interpreter. runCommand() uses CreateProcessW on Windows and
        // fork()/execvp() on POSIX, so no shell quoting is required at all.
        // Returns the child exit code, or 127 when the program cannot start.
        int runCommand(const std::vector<std::string>& argv,
                       const fs::path& workDir ,
                       bool quiet ) {
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

    } // namespace driver
} // namespace nexus
