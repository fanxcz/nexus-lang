static void nexus_mem_range_error(const char* operation, int64_t address, int64_t bytes) {
    std::fprintf(stderr,
                 "Nexus runtime error: %s range is outside a live mem_alloc block "
                 "(address=%lld, bytes=%lld)\n",
                 operation,
                 static_cast<long long>(address),
                 static_cast<long long>(bytes));
    std::exit(1);
}

int64_t nexus_mem_alloc(int64_t bytes) {
    if (bytes <= 0)
        return 0;
    if (static_cast<uint64_t>(bytes) >
        static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        return 0;
    const size_t size = static_cast<size_t>(bytes);
    return reinterpret_cast<int64_t>(nx_managed_alloc(size, size <= kNxGcMaxScannedBlock));
}
void nexus_mem_free(int64_t ptr) {
    if (ptr)
        nx_managed_free(reinterpret_cast<void*>(static_cast<uintptr_t>(ptr)));
}

void nexus_mem_set(int64_t ptr, int64_t value, int64_t bytes) {
    if (bytes == 0)
        return;
    if (bytes < 0)
        nexus_mem_range_error("mem_set", ptr, bytes);
    std::lock_guard<std::mutex> lock(gNxManagedMutex);
    if (!nx_managed_range_is_valid_locked(ptr, bytes))
        nexus_mem_range_error("mem_set", ptr, bytes);
    std::memset(reinterpret_cast<void*>(static_cast<uintptr_t>(ptr)),
                int(value) & 0xff,
                static_cast<size_t>(bytes));
}

int64_t nexus_mem_read(int64_t address) {
    if (!address)
        return 0;
    int64_t value = 0;
    std::lock_guard<std::mutex> lock(gNxManagedMutex);
    if (!nx_managed_range_is_valid_locked(address, static_cast<int64_t>(sizeof(value))))
        nexus_mem_range_error("mem_read", address, static_cast<int64_t>(sizeof(value)));
    std::memcpy(&value,
                reinterpret_cast<const void*>(static_cast<uintptr_t>(address)),
                sizeof(value));
    return value;
}
void nexus_mem_copy(int64_t destination, int64_t source, int64_t bytes) {
    if (bytes == 0)
        return;
    if (bytes < 0)
        nexus_mem_range_error("mem_copy destination", destination, bytes);
    std::lock_guard<std::mutex> lock(gNxManagedMutex);
    if (!nx_managed_range_is_valid_locked(destination, bytes))
        nexus_mem_range_error("mem_copy destination", destination, bytes);
    if (!nx_managed_range_is_valid_locked(source, bytes))
        nexus_mem_range_error("mem_copy source", source, bytes);
    std::memmove(reinterpret_cast<void*>(static_cast<uintptr_t>(destination)),
                 reinterpret_cast<const void*>(static_cast<uintptr_t>(source)),
                 static_cast<size_t>(bytes));
}
