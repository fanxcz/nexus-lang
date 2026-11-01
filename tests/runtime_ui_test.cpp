#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" int nexus_runtime_test_ui_key_consumption(size_t scanCode);

extern "C" int nexus_runtime_test_ui_text_edit(char* buffer,
                                                 size_t capacity,
                                                 int64_t* caret,
                                                 int64_t* anchor,
                                                 const char* inserted,
                                                 int operation,
                                                 int64_t maxBytes);

static bool expectText(const char* actual, const char* expected, const char* label) {
    if (std::strcmp(actual, expected) == 0)
        return true;
    std::fprintf(stderr, "runtime UI test failed: %s (got '%s', expected '%s')\n",
                 label, actual, expected);
    return false;
}

static bool expectPosition(int64_t caret,
                           int64_t anchor,
                           int64_t expectedCaret,
                           int64_t expectedAnchor,
                           const char* label) {
    if (caret == expectedCaret && anchor == expectedAnchor)
        return true;
    std::fprintf(stderr,
                 "runtime UI test failed: %s (caret=%lld, anchor=%lld; expected %lld,%lld)\n",
                 label,
                 static_cast<long long>(caret),
                 static_cast<long long>(anchor),
                 static_cast<long long>(expectedCaret),
                 static_cast<long long>(expectedAnchor));
    return false;
}

int main() {
    bool ok = true;

    if (nexus_runtime_test_ui_key_consumption(43) != 1 ||
        nexus_runtime_test_ui_key_consumption(80) != 1 ||
        nexus_runtime_test_ui_key_consumption(512) != -1) {
        std::fprintf(stderr, "runtime UI test failed: a keyboard edge was not consumed exactly once\n");
        ok = false;
    }

    char text[64] = "Aé🙂Z";
    int64_t caret = 1;
    int64_t anchor = 1;

    // UTF-8 left/right movement advances by full Unicode code points.
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        nullptr, 4, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: move right returned an error\n");
        ok = false;
    }
    ok &= expectPosition(caret, anchor, 3, 3, "right arrow skips a two-byte code point");

    caret = 7;
    anchor = 7;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        nullptr, 3, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: move left returned an error\n");
        ok = false;
    }
    ok &= expectPosition(caret, anchor, 3, 3, "left arrow skips a four-byte code point");

    // Backspace and Delete remove a complete code point, not one UTF-8 byte.
    std::strcpy(text, "Aé🙂Z");
    caret = 7;
    anchor = 7;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        nullptr, 1, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: Backspace returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "AéZ", "Backspace removes the preceding code point");
    ok &= expectPosition(caret, anchor, 3, 3, "Backspace moves the caret to the removed code point");

    std::strcpy(text, "Aé🙂Z");
    caret = 3;
    anchor = 3;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        nullptr, 2, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: Delete returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "AéZ", "Delete removes the next code point");
    ok &= expectPosition(caret, anchor, 3, 3, "Delete preserves the caret");

    // Typing replaces a selected UTF-8 range and leaves the caret after input.
    std::strcpy(text, "Aé🙂Z");
    caret = 7;
    anchor = 1;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        "X", 0, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: selection replacement returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "AXZ", "typing replaces the selected range");
    ok &= expectPosition(caret, anchor, 2, 2, "replacement collapses the selection after input");

    // The byte limit never truncates inside a UTF-8 sequence.
    std::strcpy(text, "ab");
    caret = 2;
    anchor = 2;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        "é🙂", 0, 5) < 0) {
        std::fprintf(stderr, "runtime UI test failed: byte-limited insertion returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "abé", "byte limit accepts only complete UTF-8 code points");

    // A selected range can be removed with an empty insertion, and an insertion
    // that cannot fit without splitting a code point leaves the current text.
    std::strcpy(text, "abcd");
    caret = 3;
    anchor = 1;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        "", 0, 32) < 0) {
        std::fprintf(stderr, "runtime UI test failed: empty replacement returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "ad", "empty insertion deletes the selected range");

    std::strcpy(text, "ab");
    caret = 2;
    anchor = 2;
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        "🙂", 0, 5) < 0) {
        std::fprintf(stderr, "runtime UI test failed: rejected UTF-8 insertion returned an error\n");
        ok = false;
    }
    ok &= expectText(text, "ab", "insertion does not split a code point at the byte limit");

    // Invalid capacities and operations fail without writing a partial result.
    std::strcpy(text, "abc");
    caret = 3;
    anchor = 3;
    if (nexus_runtime_test_ui_text_edit(text, 3, &caret, &anchor,
                                        "x", 0, 16) != -1) {
        std::fprintf(stderr, "runtime UI test failed: undersized output buffer was accepted\n");
        ok = false;
    }
    ok &= expectText(text, "abc", "failed output capacity leaves caller buffer untouched");
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        nullptr, 99, 16) != -1) {
        std::fprintf(stderr, "runtime UI test failed: unknown edit operation was accepted\n");
        ok = false;
    }
    if (nexus_runtime_test_ui_text_edit(text, sizeof(text), &caret, &anchor,
                                        "x", 0, 0) != -1) {
        std::fprintf(stderr, "runtime UI test failed: non-positive byte limit was accepted by test helper\n");
        ok = false;
    }

    if (!ok)
        return 1;
    std::puts("NEXUS runtime UI test: PASS");
    return 0;
}
