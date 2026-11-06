#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

extern "C" char* nexus_file_read(const char* path);
extern "C" bool nexus_file_read_checked(const char* path, char** output);
extern "C" int64_t nexus_file_write(const char* path, const char* data);
extern "C" void nexus_free(void* pointer);

namespace fs = std::filesystem;

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory =
        fs::temp_directory_path() / ("nexus-runtime-files-test-" + std::to_string(stamp));
    std::error_code ec;
    fs::create_directories(directory, ec);
    if (ec)
        return 1;

    const fs::path path = directory / "data.txt";
    const fs::path missing = directory / "missing.txt";
    int result = 0;

    if (nexus_file_write(path.string().c_str(), "") != 0)
        result = 2;
    char* empty = nullptr;
    if (result == 0 && !nexus_file_read_checked(path.string().c_str(), &empty))
        result = 3;
    if (result == 0 && (!empty || empty[0] != '\0'))
        result = 4;
    if (empty)
        nexus_free(empty);

    const char* expected = "NEXUS file API: round trip";
    if (result == 0 && nexus_file_write(path.string().c_str(), expected) != 0)
        result = 5;
    char* checked = nullptr;
    if (result == 0 && !nexus_file_read_checked(path.string().c_str(), &checked))
        result = 6;
    if (result == 0 && (!checked || std::strcmp(checked, expected) != 0))
        result = 7;
    if (checked)
        nexus_free(checked);

    char* ordinary = result == 0 ? nexus_file_read(path.string().c_str()) : nullptr;
    if (result == 0 && (!ordinary || std::strcmp(ordinary, expected) != 0))
        result = 8;
    if (ordinary)
        nexus_free(ordinary);

    char sentinel[] = "x";
    char* absent = sentinel;
    if (result == 0 && nexus_file_read_checked(missing.string().c_str(), &absent))
        result = 9;
    if (result == 0 && absent != nullptr)
        result = 10;
    if (result == 0 && nexus_file_read(missing.string().c_str()) != nullptr)
        result = 11;
    if (result == 0 && nexus_file_write(directory.string().c_str(), expected) != -1)
        result = 12;
    if (nexus_file_write(nullptr, expected) != -1)
        result = 13;
    if (nexus_file_read_checked(path.string().c_str(), nullptr))
        result = 14;

    fs::remove_all(directory, ec);
    if (result == 0 && ec)
        result = 15;
    if (result != 0) {
        std::fprintf(stderr, "NEXUS runtime file test failed at check %d\n", result);
        return result;
    }
    std::puts("NEXUS runtime file API: PASS");
    return 0;
}
