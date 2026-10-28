// string.len: returns byte length of string
int64_t nexus_string_len(const char* s) {
    if (!s)
        return 0;
    return static_cast<int64_t>(std::strlen(s));
}

// string.byte_at: returns byte at index (0-255), or -1 if out of bounds
int64_t nexus_string_byte_at(const char* s, int64_t index) {
    if (!s)
        return -1;
    size_t len = std::strlen(s);
    if (index < 0 || static_cast<size_t>(index) >= len)
        return -1;
    return static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(index)]));
}

// Decode one UTF-8 code point starting at `index`. Invalid bytes are consumed
// one at a time and reported as their byte value, so callers always make
// progress even on malformed input. Returns false when the sequence is invalid.
static bool nx_utf8_next(const unsigned char* s, size_t len, size_t& index, int64_t& cp) {
    if (index >= len)
        return false;
    const unsigned char first = s[index];
    if (first < 0x80) {
        cp = first;
        ++index;
        return true;
    }

    size_t extra = 0;
    int64_t value = 0;
    if ((first & 0xE0) == 0xC0) {
        extra = 1;
        value = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        extra = 2;
        value = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        extra = 3;
        value = first & 0x07;
    } else {
        cp = first;
        ++index;
        return false;
    }

    if (index + extra >= len) {
        cp = first;
        ++index;
        return false;
    }

    for (size_t k = 1; k <= extra; ++k) {
        const unsigned char next = s[index + k];
        if ((next & 0xC0) != 0x80) {
            cp = first;
            ++index;
            return false;
        }
        value = (value << 6) | (next & 0x3F);
    }

    const bool overlong = (extra == 1 && value < 0x80) || (extra == 2 && value < 0x800) ||
                          (extra == 3 && value < 0x10000);
    const bool surrogate = value >= 0xD800 && value <= 0xDFFF;
    if (overlong || surrogate || value > 0x10FFFF) {
        cp = first;
        ++index;
        return false;
    }

    cp = value;
    index += extra + 1;
    return true;
}

// string.chars: number of UTF-8 code points (invalid bytes count as one each)
int64_t nexus_string_char_count(const char* s) {
    if (!s)
        return 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t index = 0;
    int64_t count = 0;
    while (index < len) {
        int64_t cp = 0;
        (void)nx_utf8_next(bytes, len, index, cp);
        ++count;
    }
    return count;
}

// string.char_at: code point at a character index, or -1 if out of range
int64_t nexus_string_char_at(const char* s, int64_t index) {
    if (!s || index < 0)
        return -1;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t byteIndex = 0;
    int64_t charIndex = 0;
    while (byteIndex < len) {
        int64_t cp = 0;
        (void)nx_utf8_next(bytes, len, byteIndex, cp);
        if (charIndex == index)
            return cp;
        ++charIndex;
    }
    return -1;
}

// string.valid_utf8: true when every byte sequence is a valid UTF-8 code point
bool nexus_string_valid_utf8(const char* s) {
    if (!s)
        return true;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t index = 0;
    while (index < len) {
        int64_t cp = 0;
        if (!nx_utf8_next(bytes, len, index, cp))
            return false;
    }
    return true;
}

// string.from_char: encode a Unicode code point (invalid values use U+FFFD)
char* nexus_string_from_char(int64_t code) {
    if (code < 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        code = 0xFFFD;

    char out[4];
    size_t n = 0;
    if (code < 0x80) {
        out[n++] = static_cast<char>(code);
    } else if (code < 0x800) {
        out[n++] = static_cast<char>(0xC0 | (code >> 6));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        out[n++] = static_cast<char>(0xE0 | (code >> 12));
        out[n++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    } else {
        out[n++] = static_cast<char>(0xF0 | (code >> 18));
        out[n++] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out[n++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    }

    char* result = (char*)nx_managed_alloc(n + 1);
    if (!result)
        return nx_strdup("");
    std::memcpy(result, out, n);
    result[n] = 0;
    return result;
}

// capture_output: run a shell command and return its stdout as a string.
char* nexus_capture_output(const char* cmd) {
    if (!cmd || !*cmd)
        return nx_strdup("");
#if defined(_WIN32)
    std::FILE* p = _popen(cmd, "rb");
#else
    std::FILE* p = popen(cmd, "r");
#endif
    if (!p)
        return nx_strdup("");
    std::string out;
    char buf[4096];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), p)) > 0)
        out.append(buf, n);
#if defined(_WIN32)
    _pclose(p);
#else
    pclose(p);
#endif
    char* result = (char*)nx_managed_alloc(out.size() + 1);
    if (!result)
        return nx_strdup("");
    std::memcpy(result, out.data(), out.size());
    result[out.size()] = 0;
    return result;
}

// string.contains: substring test
bool nexus_string_contains(const char* s, const char* sub) {
    if (!s || !sub)
        return false;
    if (!*sub)
        return true;
    return std::strstr(s, sub) != nullptr;
}

// string.find: byte index of first occurrence, or -1
int64_t nexus_string_find(const char* s, const char* sub) {
    if (!s || !sub)
        return -1;
    const char* hit = std::strstr(s, sub);
    return hit ? static_cast<int64_t>(hit - s) : -1;
}

static bool nx_is_space_byte(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

// string.trim: copy without leading/trailing ASCII whitespace
char* nexus_string_trim(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    size_t b = 0, e = len;
    while (b < e && nx_is_space_byte((unsigned char)s[b]))
        ++b;
    while (e > b && nx_is_space_byte((unsigned char)s[e - 1]))
        --e;
    char* o = (char*)nx_managed_alloc(e - b + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, s + b, e - b);
    o[e - b] = 0;
    return o;
}

// string.upper / string.lower: ASCII case conversion
char* nexus_string_upper(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    char* o = (char*)nx_managed_alloc(len + 1);
    if (!o)
        return nx_strdup("");
    for (size_t i = 0; i < len; ++i)
        o[i] = char(std::toupper((unsigned char)s[i]));
    o[len] = 0;
    return o;
}

char* nexus_string_lower(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    char* o = (char*)nx_managed_alloc(len + 1);
    if (!o)
        return nx_strdup("");
    for (size_t i = 0; i < len; ++i)
        o[i] = char(std::tolower((unsigned char)s[i]));
    o[len] = 0;
    return o;
}

// string.slice: substring by byte range, boundaries clamped to length
char* nexus_string_slice(const char* s, int64_t start, int64_t end) {
    if (!s)
        return nx_strdup("");
    int64_t len = static_cast<int64_t>(std::strlen(s));
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;
    if (start >= end)
        return nx_strdup("");
    char* o = (char*)nx_managed_alloc(size_t(end - start) + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, s + start, size_t(end - start));
    o[end - start] = 0;
    return o;
}

// string.split_count: number of separator-separated fields (>=1)
int64_t nexus_string_split_count(const char* s, const char* sep) {
    if (!s)
        return 1;
    if (!sep || !*sep)
        return 1;
    int64_t count = 1;
    const char* p = s;
    size_t sl = std::strlen(sep);
    while ((p = std::strstr(p, sep)) != nullptr) {
        ++count;
        p += sl;
    }
    return count;
}

// string.split_at: field `index` (0-based) after splitting by separator
char* nexus_string_split_at(const char* s, const char* sep, int64_t index) {
    if (!s)
        return nx_strdup("");
    if (!sep || !*sep || index < 0)
        return nx_strdup("");
    size_t sl = std::strlen(sep);
    const char* p = s;
    for (int64_t i = 0; i < index; ++i) {
        const char* hit = std::strstr(p, sep);
        if (!hit)
            return nx_strdup("");
        p = hit + sl;
    }
    const char* hit = std::strstr(p, sep);
    size_t fieldLen = hit ? size_t(hit - p) : std::strlen(p);
    char* o = (char*)nx_managed_alloc(fieldLen + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, p, fieldLen);
    o[fieldLen] = 0;
    return o;
}

// Growable lists behind opaque i64 handles. Values live until the process
