#include "driver/driver_internal.hpp"

namespace nexus {
    namespace driver {


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

    } // namespace driver
} // namespace nexus
