static std::vector<std::string> get_os_args() {
    std::vector<std::string> args;
#if defined(_WIN32)
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wargv && argc > 0) {
        for (int i = 1; i < argc; ++i) { // Skip executable name (index 0)
            int size = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
            if (size > 0) {
                std::string arg(static_cast<size_t>(size), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, &arg[0], size, nullptr, nullptr);
                if (!arg.empty() && arg.back() == '\0')
                    arg.pop_back();
                args.push_back(std::move(arg));
            }
        }
        LocalFree(wargv);
    }
#elif defined(__APPLE__)
    // macOS does not expose Linux's /proc/self/cmdline. Use the system
    // argument vector and skip argv[0], matching the Windows/Linux API.
    int argc = *_NSGetArgc();
    char** argv = *_NSGetArgv();
    for (int i = 1; argv && i < argc; ++i) {
        args.emplace_back(argv[i] ? argv[i] : "");
    }
#else
    // Linux: /proc/self/cmdline is a NUL-separated argument vector.
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    if (cmdline) {
        std::string arg;
        char ch;
        bool first = true;
        while (cmdline.get(ch)) {
            if (ch == '\0') {
                if (!first)
                    args.push_back(arg);
                first = false;
                arg.clear();
            } else {
                arg += ch;
            }
        }
        if (!first && !arg.empty())
            args.push_back(arg);
    }
#endif
    return args;
}
