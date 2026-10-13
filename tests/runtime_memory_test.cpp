#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" char* nexus_str_i64(int64_t value);
extern "C" char* nexus_str_concat(const char* left, const char* right);
extern "C" void nexus_free(void* pointer);
extern "C" size_t nexus_runtime_test_live_bytes();
extern "C" void nexus_runtime_test_collect();

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

    nexus_free(persistent);
    nexus_free(latest);
    nexus_runtime_test_collect();
    return 0;
}
