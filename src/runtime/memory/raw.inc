int64_t nexus_mem_alloc(int64_t bytes) {
    if (bytes <= 0)
        return 0;
    return reinterpret_cast<int64_t>(nx_managed_alloc(
        static_cast<size_t>(bytes), static_cast<size_t>(bytes) <= kNxGcMaxScannedBlock));
}
void nexus_mem_free(int64_t ptr) {
    if (ptr)
        nx_managed_free(reinterpret_cast<void*>(ptr));
}

void nexus_mem_set(int64_t ptr, int64_t value, int64_t bytes) {
    if (ptr == 0 || bytes <= 0)
        return;
    std::memset(reinterpret_cast<void*>(ptr), int(value) & 0xff, static_cast<size_t>(bytes));
}

int64_t nexus_mem_read(int64_t address) {
    if (!address)
        return 0;
    return *reinterpret_cast<int64_t*>(address);
}
void nexus_mem_copy(int64_t destination, int64_t source, int64_t bytes) {
    if (destination == 0 || source == 0 || bytes <= 0)
        return;
    std::memcpy(reinterpret_cast<void*>(destination),
                reinterpret_cast<const void*>(source),
                static_cast<size_t>(bytes));
}
