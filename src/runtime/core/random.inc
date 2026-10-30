static uint64_t nexus_runtime_random_u64() {
    // SplitMix64: a compact non-cryptographic generator. Atomic state makes
    // concurrent calls well-defined; do not use random_i64 for secrets.
    constexpr uint64_t increment = UINT64_C(0x9E3779B97F4A7C15);
    static std::atomic<uint64_t> state{
        static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (static_cast<uint64_t>(std::time(nullptr)) + increment)};
    uint64_t value = state.fetch_add(increment, std::memory_order_relaxed) + increment;
    value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
    return value ^ (value >> 31);
}

static int64_t nexus_i64_add_unsigned_offset(int64_t base, uint64_t offset) {
    if (base >= 0)
        return base + static_cast<int64_t>(offset);

    // Distance to zero is calculated without negating INT64_MIN.
    const uint64_t untilZero = static_cast<uint64_t>(-(base + 1)) + 1u;
    if (offset < untilZero)
        return base + static_cast<int64_t>(offset);
    return static_cast<int64_t>(offset - untilZero);
}

int64_t nexus_random_i64(int64_t min, int64_t max) {
    if (min > max)
        std::swap(min, max);

    // Unsigned arithmetic avoids signed overflow, including the complete i64
    // domain where the inclusive span is 2^64 and wraps to zero.
    const uint64_t span = static_cast<uint64_t>(max) -
                          static_cast<uint64_t>(min) + uint64_t{1};
    uint64_t offset = 0;
    if (span == 0) {
        offset = nexus_runtime_random_u64();
    } else {
        // Rejection sampling removes modulo bias for bounded intervals.
        const uint64_t threshold = (uint64_t{0} - span) % span;
        uint64_t randomValue = 0;
        do {
            randomValue = nexus_runtime_random_u64();
        } while (randomValue < threshold);
        offset = randomValue % span;
    }
    return nexus_i64_add_unsigned_offset(min, offset);
}
int64_t nexus_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
