char* nexus_http_get(const char* url) {
    if (!url || !*url)
        return nx_strdup("");

    // Keep the response bounded: this API returns a single managed string and
    // has no streaming interface. Pass the URL as an argv item, never through a
    // command shell (notably, single-quote escaping is not safe for cmd.exe).
    constexpr size_t maxResponseBytes = 16u * 1024u * 1024u;
#if defined(_WIN32)
    const int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, nullptr, 0);
    if (wideLength <= 1)
        return nx_strdup("");
    std::wstring wideUrl(static_cast<size_t>(wideLength), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1,
                            wideUrl.data(), wideLength) == 0)
        return nx_strdup("");
    wideUrl.pop_back();

    const auto quoteArgument = [](const std::wstring& argument) {
        std::wstring quoted(1, L'"');
        size_t backslashes = 0;
        for (const wchar_t c : argument) {
            if (c == L'\\') {
                ++backslashes;
            } else if (c == L'"') {
                quoted.append(backslashes * 2u + 1u, L'\\');
                quoted.push_back(L'"');
                backslashes = 0;
            } else {
                quoted.append(backslashes, L'\\');
                quoted.push_back(c);
                backslashes = 0;
            }
        }
        quoted.append(backslashes * 2u, L'\\');
        quoted.push_back(L'"');
        return quoted;
    };

    const std::wstring command =
        quoteArgument(L"curl.exe") + L" -fsSL --max-time 15 -- " + quoteArgument(wideUrl);
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &attributes, 0))
        return nx_strdup("");
    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        return nx_strdup("");
    }
    HANDLE nullInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullInput == INVALID_HANDLE_VALUE) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        return nx_strdup("");
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nullInput;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(nullInput);
    CloseHandle(writePipe);
    if (!created) {
        CloseHandle(readPipe);
        return nx_strdup("");
    }

    std::string output;
    char buffer[4096];
    bool tooLarge = false;
    bool readFailed = false;
    for (;;) {
        DWORD received = 0;
        if (!ReadFile(readPipe, buffer, static_cast<DWORD>(sizeof(buffer)), &received, nullptr)) {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                readFailed = true;
            break;
        }
        if (received == 0)
            break;
        if (output.size() > maxResponseBytes - static_cast<size_t>(received)) {
            tooLarge = true;
            break;
        }
        output.append(buffer, static_cast<size_t>(received));
    }
    CloseHandle(readPipe);
    if (tooLarge || readFailed)
        TerminateProcess(process.hProcess, 1);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (tooLarge || readFailed || exitCode != 0)
        return nx_strdup("");
    return nx_strdup(output.c_str());
#else
    int pipes[2] = {-1, -1};
    if (pipe(pipes) != 0)
        return nx_strdup("");

    const pid_t child = fork();
    if (child < 0) {
        close(pipes[0]);
        close(pipes[1]);
        return nx_strdup("");
    }
    if (child == 0) {
        close(pipes[0]);
        if (dup2(pipes[1], STDOUT_FILENO) < 0)
            _exit(126);
        close(pipes[1]);
        execlp("curl", "curl", "-fsSL", "--max-time", "15", "--", url,
               static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipes[1]);
    std::string output;
    char buffer[4096];
    bool tooLarge = false;
    bool readFailed = false;
    for (;;) {
        const ssize_t received = read(pipes[0], buffer, sizeof(buffer));
        if (received == 0)
            break;
        if (received < 0) {
            if (errno == EINTR)
                continue;
            readFailed = true;
            break;
        }
        if (output.size() > maxResponseBytes - static_cast<size_t>(received)) {
            tooLarge = true;
            break;
        }
        output.append(buffer, static_cast<size_t>(received));
    }
    close(pipes[0]);
    if (tooLarge || readFailed)
        kill(child, SIGTERM);
    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            status = -1;
            break;
        }
    }
    if (tooLarge || readFailed || status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return nx_strdup("");
    return nx_strdup(output.c_str());
#endif
}
