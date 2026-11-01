#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

extern "C" char* nexus_os_config_dir(const char* appName);
extern "C" char* nexus_os_config_error();
extern "C" void nexus_free(void* pointer);

static bool setConfigBase(const std::string& value) {
#if defined(_WIN32)
    return _putenv_s("APPDATA", value.c_str()) == 0;
#elif defined(__APPLE__)
    return setenv("HOME", value.c_str(), 1) == 0;
#else
    return setenv("XDG_CONFIG_HOME", value.c_str(), 1) == 0;
#endif
}

static bool expect(bool condition, const char* message) {
    if (condition)
        return true;
    std::fprintf(stderr, "runtime OS test failed: %s\n", message);
    return false;
}

static std::string takeString(char* value) {
    std::string result = value ? value : "";
    if (value)
        nexus_free(value);
    return result;
}

int main() {
    namespace fs = std::filesystem;
    bool ok = true;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() /
        ("nexus-os-config-test-" + std::to_string(nonce));
    std::error_code ec;
    fs::create_directories(root, ec);
    if (ec) {
        std::fprintf(stderr, "runtime OS test failed: cannot create fixture: %s\n", ec.message().c_str());
        return 1;
    }

    const std::string rootText = root.string();
    ok &= expect(setConfigBase(rootText), "could not set a temporary platform config base");

    const std::string expected =
#if defined(__APPLE__)
        (root / "Library" / "Application Support" / "NEXUS Test App").lexically_normal().string();
#else
        (root / "NEXUS Test App").lexically_normal().string();
#endif
    const std::string actual = takeString(nexus_os_config_dir("NEXUS Test App"));
    ok &= expect(actual == expected, "config_dir returned the wrong platform-specific path");
    ok &= expect(fs::is_directory(expected), "config_dir did not create the application directory");
    ok &= expect(takeString(nexus_os_config_error()).empty(),
                 "successful config_dir call left a stale error");

    const char* invalidNames[] = {
        "", ".", "..", "../escape", "folder/name", "folder\\name", "bad:name", "bad|name"
    };
    for (const char* name : invalidNames) {
        const std::string result = takeString(nexus_os_config_dir(name));
        if (!result.empty()) {
            std::fprintf(stderr, "runtime OS test failed: accepted unsafe app name '%s' as '%s'\n",
                         name, result.c_str());
            ok = false;
        }
        const std::string error = takeString(nexus_os_config_error());
        if (error.empty()) {
            std::fprintf(stderr, "runtime OS test failed: invalid app name '%s' had no error\n", name);
            ok = false;
        }
    }

    const std::string longName(129, 'x');
    ok &= expect(takeString(nexus_os_config_dir(longName.c_str())).empty(),
                 "config_dir accepted an application name longer than 128 bytes");
    ok &= expect(!takeString(nexus_os_config_error()).empty(),
                 "overlong application name did not report an error");
    ok &= expect(!fs::exists(root.parent_path() / "escape"),
                 "path traversal created a directory outside the config root");

    // A regular file cannot be used as the base config directory.
    const fs::path notDirectory = root / "not-a-directory";
    {
        FILE* file = std::fopen(notDirectory.string().c_str(), "wb");
        if (file) {
            std::fputs("sentinel", file);
            std::fclose(file);
        } else {
            ok &= expect(false, "could not create the invalid config-base fixture");
        }
    }
    ok &= expect(setConfigBase(notDirectory.string()), "could not set invalid config base");
    ok &= expect(takeString(nexus_os_config_dir("NEXUS")).empty(),
                 "config_dir accepted a file as the config base");
    ok &= expect(!takeString(nexus_os_config_error()).empty(),
                 "config base failure did not report an error");

    setConfigBase(rootText);
    fs::remove_all(root, ec);
    if (ec) {
        std::fprintf(stderr, "runtime OS test failed: cleanup failed: %s\n", ec.message().c_str());
        ok = false;
    }

    if (!ok)
        return 1;
    std::puts("NEXUS runtime OS test: PASS");
    return 0;
}
