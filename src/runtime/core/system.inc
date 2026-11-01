int64_t nexus_system(const char* c) {
    return c ? std::system(c) : -1;
}

static thread_local std::string gNxOsConfigError;

static char* nexus_os_copy_string(const std::string& value) {
    char* result = static_cast<char*>(nx_managed_alloc(value.size() + 1u));
    if (!result)
        return nullptr;
    std::memcpy(result, value.c_str(), value.size() + 1u);
    return result;
}

static bool nexus_os_valid_app_name(const char* appName, std::string& result) {
    if (!appName) {
        gNxOsConfigError = "application name is null";
        return false;
    }
    result.assign(appName);
    if (result.empty()) {
        gNxOsConfigError = "application name is empty";
        return false;
    }
    if (result.size() > 128u) {
        gNxOsConfigError = "application name exceeds 128 bytes";
        return false;
    }
    if (result == "." || result == "..") {
        gNxOsConfigError = "application name must not be a dot path component";
        return false;
    }
    for (const unsigned char ch : result) {
        if (ch < 0x20u || ch == 0x7fu ||
            ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' ||
            ch == '"' || ch == '<' || ch == '>' || ch == '|') {
            gNxOsConfigError = "application name contains a path separator or invalid filename character";
            return false;
        }
    }
#if defined(_WIN32)
    if (result.back() == '.' || result.back() == ' ') {
        gNxOsConfigError = "application name must not end with a dot or space on Windows";
        return false;
    }
#endif
    return true;
}

char* nexus_os_config_dir(const char* appName) {
    gNxOsConfigError.clear();
    std::string validatedName;
    if (!nexus_os_valid_app_name(appName, validatedName))
        return nexus_os_copy_string("");

    std::filesystem::path base;
#if defined(_WIN32)
    const char* baseValue = std::getenv("APPDATA");
    if (!baseValue || !*baseValue)
        baseValue = std::getenv("LOCALAPPDATA");
    if (!baseValue || !*baseValue) {
        gNxOsConfigError = "APPDATA and LOCALAPPDATA are not set";
        return nexus_os_copy_string("");
    }
    base = std::filesystem::path(baseValue);
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (!home || !*home) {
        gNxOsConfigError = "HOME is not set";
        return nexus_os_copy_string("");
    }
    base = std::filesystem::path(home) / "Library" / "Application Support";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        const std::filesystem::path xdgPath(xdg);
        if (xdgPath.is_absolute()) {
            base = xdgPath;
        }
    }
    if (base.empty()) {
        const char* home = std::getenv("HOME");
        if (!home || !*home) {
            gNxOsConfigError = "XDG_CONFIG_HOME must be absolute or HOME must be set";
            return nexus_os_copy_string("");
        }
        base = std::filesystem::path(home) / ".config";
    }
#endif

    if (!base.is_absolute()) {
        gNxOsConfigError = "platform configuration base directory is not absolute";
        return nexus_os_copy_string("");
    }

    const std::filesystem::path destination = base / std::filesystem::path(validatedName);
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    if (ec) {
        gNxOsConfigError = "cannot create application configuration directory: " + ec.message();
        return nexus_os_copy_string("");
    }
    const bool isDirectory = std::filesystem::is_directory(destination, ec);
    if (ec || !isDirectory) {
        gNxOsConfigError = ec
            ? "cannot inspect application configuration directory: " + ec.message()
            : "application configuration path is not a directory";
        return nexus_os_copy_string("");
    }

    return nexus_os_copy_string(destination.lexically_normal().string());
}

char* nexus_os_config_error() {
    return nexus_os_copy_string(gNxOsConfigError);
}
