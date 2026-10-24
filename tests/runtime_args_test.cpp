#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" int64_t nexus_os_args_count();
extern "C" char* nexus_os_args_get(int64_t index);
extern "C" void nexus_free(void* pointer);

int main(int argc, char** argv) {
    const int64_t expectedCount = static_cast<int64_t>(argc - 1);
    const int64_t actualCount = nexus_os_args_count();
    if (actualCount != expectedCount) {
        std::fprintf(stderr,
                     "argument count mismatch: got %lld, expected %lld\n",
                     static_cast<long long>(actualCount),
                     static_cast<long long>(expectedCount));
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        char* actual = nexus_os_args_get(static_cast<int64_t>(i - 1));
        const char* expected = argv[i];
        if (!actual || std::strcmp(actual, expected) != 0) {
            std::fprintf(stderr,
                         "argument %d mismatch: got '%s', expected '%s'\n",
                         i - 1,
                         actual ? actual : "<null>",
                         expected);
            if (actual)
                nexus_free(actual);
            return 2;
        }
        nexus_free(actual);
    }

    char* negativeIndex = nexus_os_args_get(-1);
    if (!negativeIndex || negativeIndex[0] != '\0') {
        std::fprintf(stderr, "negative argument index should return an empty string\n");
        if (negativeIndex)
            nexus_free(negativeIndex);
        return 3;
    }
    nexus_free(negativeIndex);
    char* outOfRange = nexus_os_args_get(expectedCount);
    if (!outOfRange || outOfRange[0] != '\0') {
        std::fprintf(stderr, "out-of-range argument index should return an empty string\n");
        if (outOfRange)
            nexus_free(outOfRange);
        return 4;
    }
    nexus_free(outOfRange);
    return 0;
}
