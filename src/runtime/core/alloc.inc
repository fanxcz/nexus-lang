
#ifdef NEXUS_RUNTIME_TESTING
size_t nexus_runtime_test_live_bytes() {
    return nx_test_live_managed_bytes();
}
void nexus_runtime_test_collect() {
    nx_gc_collect();
}
int nexus_runtime_test_try_free(void* pointer) {
    return nx_managed_try_free(pointer) ? 1 : 0;
}
int nexus_runtime_test_range_valid(int64_t address, int64_t bytes) {
    return nx_managed_range_is_valid(address, bytes) ? 1 : 0;
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
int64_t nexus_division_check(int64_t dividend, int64_t divisor) {
    if (divisor == 0) {
        std::fprintf(stderr, "NEXUS runtime error: division by zero\n");
        std::exit(1);
    }
    if (dividend == INT64_MIN && divisor == -1) {
        std::fprintf(stderr, "NEXUS runtime error: signed integer division overflow\n");
        std::exit(1);
    }
    return divisor;
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
