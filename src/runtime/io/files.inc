char* nexus_file_read(const char* p) {
    if (!p)
        return nullptr;
    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return nullptr;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::rewind(f);
    if (n < 0) {
        std::fclose(f);
        return nullptr;
    }
    char* o = (char*)nx_managed_alloc(size_t(n) + 1);
    if (!o) {
        std::fclose(f);
        return nullptr;
    }
    size_t got = std::fread(o, 1, size_t(n), f);
    std::fclose(f);
    o[got] = 0;
    return o;
}
// Checked file read: success is independent of the returned text length, so an
// existing empty file is distinct from an open/read/allocation failure.
bool nexus_file_read_checked(const char* p, char** out) {
    if (!out)
        return false;
    *out = nullptr;
    if (!p)
        return false;

    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return false;
    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return false;
    }
    const long length = std::ftell(f);
    if (length < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    const auto size = static_cast<unsigned long long>(length);
    if (size >= static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) {
        std::fclose(f);
        return false;
    }

    char* data = static_cast<char*>(nx_managed_alloc(static_cast<size_t>(length) + 1));
    if (!data) {
        std::fclose(f);
        return false;
    }
    const size_t expected = static_cast<size_t>(length);
    const size_t read = std::fread(data, 1, expected, f);
    bool ok = read == expected && std::ferror(f) == 0;
    if (std::fclose(f) != 0)
        ok = false;
    if (!ok) {
        nx_managed_free(data);
        return false;
    }
    data[read] = '\0';
    *out = data;
    return true;
}

int64_t nexus_file_write(const char* p, const char* d) {
    if (!p)
        return -1;
    std::FILE* f = std::fopen(p, "wb");
    if (!f)
        return -1;
    const char* s = d ? d : "";
    size_t n = std::strlen(s), w = std::fwrite(s, 1, n, f);
    int ok = w == n && std::fclose(f) == 0;
    return ok ? 0 : -1;
}
bool nexus_file_exists(const char* p) {
    if (!p)
        return false;
    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return false;
    std::fclose(f);
    return true;
}
char* nexus_env(const char* n) {
    const char* v = n ? std::getenv(n) : nullptr;
    return nx_strdup(v ? v : "");
}
void nexus_exit(int64_t c) {
    std::exit(int(c));
}
