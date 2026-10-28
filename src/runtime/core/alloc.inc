
#ifdef NEXUS_RUNTIME_TESTING
size_t nexus_runtime_test_live_bytes() {
    return nx_test_live_managed_bytes();
}
void nexus_runtime_test_collect() {
    nx_gc_collect();
}
#endif

void* nexus_alloc(size_t n) {
    return nx_managed_alloc(n, n <= kNxGcMaxScannedBlock);
}
void nexus_free(void* p) {
    nx_managed_free(p);
}
void nexus_bounds_check(int64_t i, int64_t n) {
    if (i < 0 || i >= n) {
        std::fprintf(stderr,
                     "Nexus runtime error: array index %lld out of bounds for size %lld\n",
                     (long long)i,
                     (long long)n);
        std::exit(1);
    }
}
void nexus_division_check(int64_t dividend, int64_t divisor) {
    (void)dividend;
    if (divisor == 0) {
        std::fprintf(stderr, "NEXUS runtime error: division by zero\n");
        std::exit(1);
    }
}
char* nexus_str_concat(const char* a, const char* b) {
    if (!a)
        a = "";
    if (!b)
        b = "";
    size_t la = std::strlen(a), lb = std::strlen(b);
    char* out = (char*)nx_managed_alloc(la + lb + 1);
    if (!out)
        return nullptr;
    std::memcpy(out, a, la);
    std::memcpy(out + la, b, lb);
    out[la + lb] = 0;
    return out;
}
