#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

extern "C" char* nexus_str_i64(int64_t value);
extern "C" char* nexus_str_concat(const char* left, const char* right);
extern "C" void nexus_free(void* pointer);
extern "C" size_t nexus_runtime_test_live_bytes();
extern "C" void nexus_runtime_test_collect();
extern "C" int nexus_runtime_test_try_free(void* pointer);
extern "C" int nexus_runtime_test_range_valid(int64_t address, int64_t bytes);
extern "C" int64_t nexus_mem_alloc(int64_t bytes);
extern "C" void nexus_mem_free(int64_t pointer);
extern "C" void nexus_mem_set(int64_t pointer, int64_t value, int64_t bytes);
extern "C" int64_t nexus_mem_read(int64_t address);
extern "C" void nexus_mem_copy(int64_t destination, int64_t source, int64_t bytes);

int main() {
    // These strings must remain valid across many collection cycles.
    char* volatile persistent = nexus_str_concat("persistent ", "value");
    char* volatile latest = nullptr;

    for (int64_t i = 0; i < 200000; ++i) {
        char* number = nexus_str_i64(i);
        char* value = nexus_str_concat(number, "-frame");
        if (i == 199999)
            latest = value;
    }

    nexus_runtime_test_collect();

    if (std::strcmp(persistent, "persistent value") != 0 ||
        std::strcmp(latest, "199999-frame") != 0) {
        std::fprintf(stderr, "runtime GC reclaimed a live string\n");
        return 1;
    }

    const size_t liveBytes = nexus_runtime_test_live_bytes();
    if (liveBytes > 2u * 1024u * 1024u) {
        std::fprintf(stderr, "runtime string heap remained too large: %zu bytes\n", liveBytes);
        return 2;
    }

    char* freedProbe = nexus_str_concat("freed ", "probe");
    if (!freedProbe) {
        std::fprintf(stderr, "runtime allocation probe failed\n");
        return 3;
    }
    nexus_free(freedProbe);
    if (nexus_runtime_test_try_free(freedProbe) != 0) {
        std::fprintf(stderr, "runtime accepted a double free\n");
        return 4;
    }

    int stackValue = 0;
    if (nexus_runtime_test_try_free(&stackValue) != 0) {
        std::fprintf(stderr, "runtime accepted a non-managed pointer free\n");
        return 5;
    }

    const int64_t buffer = nexus_mem_alloc(16);
    if (!buffer || !nexus_runtime_test_range_valid(buffer, 16) ||
        !nexus_runtime_test_range_valid(buffer + 8, 8) ||
        nexus_runtime_test_range_valid(buffer + 12, 8) ||
        nexus_runtime_test_range_valid(buffer + 16, 1) ||
        nexus_runtime_test_range_valid(buffer, std::numeric_limits<int64_t>::max()) ||
        nexus_runtime_test_range_valid(buffer, -1) ||
        nexus_runtime_test_range_valid(0, 8)) {
        std::fprintf(stderr, "runtime memory range validation failed\n");
        return 6;
    }
    nexus_mem_set(buffer, 0x5A, 16);
    const int64_t repeatedBytePattern = INT64_C(0x5A5A5A5A5A5A5A5A);
    if (nexus_mem_read(buffer + 8) != repeatedBytePattern) {
        std::fprintf(stderr, "checked memory read/write returned wrong data\n");
        return 7;
    }
    // Overlap is defined for mem_copy; it behaves like memmove.
    nexus_mem_copy(buffer + 4, buffer, 8);
    if (nexus_mem_read(buffer + 4) != repeatedBytePattern) {
        std::fprintf(stderr, "checked overlapping memory copy returned wrong data\n");
        return 8;
    }
    nexus_mem_free(buffer);

    nexus_free(persistent);
    nexus_free(latest);
    nexus_runtime_test_collect();
    return 0;
}
