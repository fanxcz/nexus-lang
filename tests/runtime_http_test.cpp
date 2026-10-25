#include <cstdio>
#include <cstring>

extern "C" char* nexus_http_get(const char* url);
extern "C" void nexus_free(void* pointer);

int main() {
    // On Windows, cmd.exe treats '&' as a command separator even when a URL
    // is wrapped in POSIX single quotes. This invalid URL must be passed to
    // curl as one argument and must never execute the text after '&'.
    char* response = nexus_http_get("not-a-valid-url&echo NEXUS_HTTP_SHELL_INJECTION");
    if (!response) {
        std::fprintf(stderr, "HTTP helper returned a null pointer\n");
        return 1;
    }
    const bool injected = std::strstr(response, "NEXUS_HTTP_SHELL_INJECTION") != nullptr;
    nexus_free(response);
    if (injected) {
        std::fprintf(stderr, "HTTP helper interpreted URL input as a shell command\n");
        return 2;
    }
    return 0;
}
