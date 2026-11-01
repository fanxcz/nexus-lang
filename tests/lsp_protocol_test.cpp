#include "driver/lsp/protocol.hpp"

#include <cstdio>
#include <string>

namespace {
    int fail(const char* message) {
        std::fprintf(stderr, "FAIL LSP protocol: %s\n", message);
        return 1;
    }
}

int main() {
    using namespace nexus::driver::lsp;

    const std::string escaped =
        R"({"text":"line\nquote: \"ok\" slash: \/ tab: \t back: \\ end"})";
    if (extractString(escaped, "text") != "line\nquote: \"ok\" slash: / tab: \t back: \\ end")
        return fail("standard JSON string escapes were not decoded");

    const std::string unicode = R"({"text":"caf\u00e9 \uD83D\uDE00"})";
    if (extractString(unicode, "text") != std::string("caf\xc3\xa9 \xf0\x9f\x98\x80"))
        return fail("BMP escapes or UTF-16 surrogate pairs were not decoded to UTF-8");

    if (!extractString(R"({"text":""})", "text").empty())
        return fail("empty JSON string did not decode to empty text");
    const std::string decoyUri =
        R"({"text":"fake \u0022uri\u0022:\u0022file:///wrong.nx\u0022","params":{"textDocument":{"uri":"file:///right.nx"}}})";
    if (extractString(decoyUri, "uri") != "file:///right.nx")
        return fail("a field-like string value was mistaken for a JSON key");


    if (!extractString(R"({"text":"bad\uD800x"})", "text").empty())
        return fail("unpaired high surrogate was accepted");
    if (!extractString(R"({"text":"bad\uDC00"})", "text").empty())
        return fail("unpaired low surrogate was accepted");
    if (!extractString(R"({"text":"bad\u12XZ"})", "text").empty())
        return fail("invalid Unicode hex escape was accepted");

    if (extractId(R"({"jsonrpc":"2.0","id":42,"method":"x"})") != "42")
        return fail("numeric JSON-RPC id was not preserved");
    if (extractId(R"({"jsonrpc":"2.0","id":"req-7","method":"x"})") != "\"req-7\"")
        return fail("string JSON-RPC id was not preserved");
    if (extractNumber(R"({"line":12,"character":34})", "character") != 34)
        return fail("numeric field extraction failed");
    if (extractNumber(R"({"line":-1})", "line") != 0)
        return fail("negative position was not rejected");
    if (extractNumber(R"({"text":"fake \u0022line\u0022:999","position":{"line":1}})", "line") != 1)
        return fail("a field-like string value was mistaken for a JSON key");

    if (!supportsWatchedFilesRegistration(
            R"({"capabilities":{"workspace":{"didChangeWatchedFiles":{"dynamicRegistration":true}}}})"))
        return fail("dynamic watched-file registration capability was missed");
    if (supportsWatchedFilesRegistration(
            R"({"capabilities":{"workspace":{"didChangeWatchedFiles":{"dynamicRegistration":false}}}})"))
        return fail("static watched-file capability was misdetected as dynamic");
    if (supportsWatchedFilesRegistration(
            R"({"text":"fake \u0022workspace\u0022:{\u0022didChangeWatchedFiles\u0022:{\u0022dynamicRegistration\u0022:true}}","capabilities":{}})"))
        return fail("a field-like string value was mistaken for a watched-file capability");

    const std::string escapedOutput = jsonEscape("line\n\t\"\\");
    if (escapedOutput != "line\\n\\t\\\"\\\\")
        return fail("JSON output escaping is incorrect");

    const std::string controls = std::string("a") + char(1) + '\b' + '\f' + char(11);
    if (jsonEscape(controls) != "a\\u0001\\b\\f\\u000b")
        return fail("JSON output did not escape all C0 control characters");

    std::puts("NEXUS LSP protocol tests: PASS");
    return 0;
}
