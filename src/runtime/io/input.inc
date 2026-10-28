bool nexus_str_equal(const char* a, const char* b) {
    return a == b || (!a && !b) || (a && b && std::strcmp(a, b) == 0);
}
static char* nx_readline() {
    size_t cap = 256, len = 0;
    char* buf = (char*)nx_managed_alloc(cap);
    if (!buf)
        return nullptr;
    int ch;
    while ((ch = std::getchar()) != EOF) {
        if (ch == '\r')
            continue;
        if (ch == '\n')
            break;
        if (len + 1 >= cap) {
            cap *= 2;
            char* nb = (char*)nx_managed_realloc(buf, cap);
            if (!nb) {
                nx_managed_free(buf);
                return nullptr;
            }
            buf = nb;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = 0;
    return buf;
}
char* nexus_input(const char* p) {
    if (p)
        std::fputs(p, stdout);
    std::fflush(stdout);
    return nx_readline();
}
int64_t nexus_input_i64(const char* p) {
    for (;;) {
        char* s = nexus_input(p);
        if (!s)
            return 0;
        char* e = nullptr;
        long long v = std::strtoll(s, &e, 10);
        while (e && *e == ' ')
            ++e;
        if (e && *e == 0) {
            nx_managed_free(s);
            return v;
        }
        std::fprintf(stderr, "Invalid integer.\n");
        nx_managed_free(s);
    }
}
double nexus_input_f64(const char* p) {
    for (;;) {
        char* s = nexus_input(p);
        if (!s)
            return 0;
        char* e = nullptr;
        double v = std::strtod(s, &e);
        while (e && *e == ' ')
            ++e;
        if (e && *e == 0) {
            nx_managed_free(s);
            return v;
        }
        std::fprintf(stderr, "Invalid number.\n");
        nx_managed_free(s);
    }
}
char* nexus_str_i64(int64_t v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%lld", (long long)v);
    return nx_strdup(b);
}
char* nexus_str_f64(double v) {
    char b[96];
    std::snprintf(b, sizeof(b), "%.15g", v);
    return nx_strdup(b);
}
char* nexus_str_bool(bool v) {
    return nx_strdup(v ? "true" : "false");
}
void nexus_clear() {
    std::fputs("\033[2J\033[H", stdout);
    std::fflush(stdout);
}
void nexus_sleep_ms(int64_t ms) {
    if (ms <= 0)
        return;

#if defined(_WIN32)
    Sleep((DWORD)ms);

#else
    timespec ts{};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)((ms % 1000) * 1000000);
    nanosleep(&ts, nullptr);

#endif
}
