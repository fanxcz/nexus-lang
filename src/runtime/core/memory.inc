    struct NxManagedBlock {
        size_t size = 0;
        bool scanContents = false;
        std::thread::id owner;
    };
    struct NxManagedBlockView {
        uintptr_t base = 0;
        void* pointer = nullptr;
        size_t size = 0;
        bool scanContents = false;
        bool marked = false;
    };
    static std::unordered_map<void*, NxManagedBlock> gNxManagedBlocks;
    static std::mutex gNxManagedMutex;
    static constexpr size_t kNxGcThreshold = 512u * 1024u;
    static constexpr size_t kNxGcMaxScannedBlock = 1024u * 1024u;
    static thread_local size_t gNxBytesSinceCollection = 0;
    static thread_local bool gNxCollecting = false;
    static uintptr_t nx_stack_high() {
#if defined(_WIN32)
        auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
        return tib ? reinterpret_cast<uintptr_t>(tib->StackBase) : 0;
#elif defined(__APPLE__)
        return reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
#elif defined(__linux__)
        pthread_attr_t attr;
        if (pthread_getattr_np(pthread_self(), &attr) != 0)
            return 0;
        void* base = nullptr;
        size_t size = 0;
        const int status = pthread_attr_getstack(&attr, &base, &size);
        pthread_attr_destroy(&attr);
        return status == 0 ? reinterpret_cast<uintptr_t>(base) + size : 0;
#else
        return 0;
#endif
    }
    static void nx_gc_collect();
    static void* nx_managed_alloc(size_t size, bool scanContents = false) {
        if (size == 0)
            size = 1;
        if (!gNxCollecting && gNxBytesSinceCollection >= kNxGcThreshold)
            nx_gc_collect();
        void* pointer = std::malloc(size);
        if (!pointer)
            return nullptr;
        {
            std::lock_guard<std::mutex> lock(gNxManagedMutex);
            gNxManagedBlocks.emplace(
                pointer, NxManagedBlock{size, scanContents, std::this_thread::get_id()});
        }
        if (size > std::numeric_limits<size_t>::max() - gNxBytesSinceCollection)
            gNxBytesSinceCollection = kNxGcThreshold;
        else
            gNxBytesSinceCollection += size;
        return pointer;
    }
    static void* nx_managed_realloc(void* pointer, size_t size) {
        if (!pointer)
            return nx_managed_alloc(size);
        if (size == 0)
            size = 1;
        if (!gNxCollecting && gNxBytesSinceCollection >= kNxGcThreshold)
            nx_gc_collect();
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        auto old = gNxManagedBlocks.find(pointer);
        void* resized = std::realloc(pointer, size);
        if (!resized)
            return nullptr;
        if (old != gNxManagedBlocks.end()) {
            NxManagedBlock block = old->second;
            gNxManagedBlocks.erase(old);
            block.size = size;
            gNxManagedBlocks[resized] = block;
        }
        if (size > std::numeric_limits<size_t>::max() - gNxBytesSinceCollection)
            gNxBytesSinceCollection = kNxGcThreshold;
        else
            gNxBytesSinceCollection += size;
        return resized;
    }
    static bool nx_managed_try_free(void* pointer) {
        if (!pointer)
            return true;
        {
            std::lock_guard<std::mutex> lock(gNxManagedMutex);
            const auto block = gNxManagedBlocks.find(pointer);
            if (block == gNxManagedBlocks.end())
                return false;
            gNxManagedBlocks.erase(block);
        }
        std::free(pointer);
        return true;
    }
    static void nx_managed_free(void* pointer) {
        if (!nx_managed_try_free(pointer)) {
            std::fputs("Nexus runtime error: invalid or already freed allocation\n", stderr);
            std::exit(1);
        }
    }
    // Conservative collection scans arbitrary machine words from the active stack.
    // ASan deliberately poisons stack red-zones, although those bytes are part of the
    // stack range this collector walks. Keep this tiny byte-loader uninstrumented;
    // the actual mark/sweep logic and heap-object scanning remain sanitizer-instrumented.
#if defined(__clang__)
    __attribute__((no_sanitize("address")))
#elif defined(__GNUC__)
    __attribute__((no_sanitize_address))
#elif defined(_MSC_VER)
    __declspec(no_sanitize_address)
#endif
    static uintptr_t
    nx_gc_read_stack_word(uintptr_t address) {
        const volatile unsigned char* input =
            reinterpret_cast<const volatile unsigned char*>(address);
        uintptr_t value = 0;
        unsigned char* output = reinterpret_cast<unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); ++i)
            output[i] = input[i];
        return value;
    }

    static void nx_gc_collect() {
        if (gNxCollecting)
            return;
        gNxCollecting = true;
        struct CollectGuard {
            ~CollectGuard() {
                gNxCollecting = false;
            }
        } guard;
        volatile uintptr_t stackMarker = 0;
        const uintptr_t stackStart = reinterpret_cast<uintptr_t>(&stackMarker);
        const uintptr_t stackEnd = nx_stack_high();
        if (stackEnd <= stackStart) {
            gNxBytesSinceCollection = 0;
            return;
        }
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        const auto owner = std::this_thread::get_id();
        std::vector<NxManagedBlockView> blocks;
        blocks.reserve(gNxManagedBlocks.size());
        for (const auto& [pointer, block] : gNxManagedBlocks) {
            if (block.owner != owner)
                continue;
            blocks.push_back({reinterpret_cast<uintptr_t>(pointer),
                              pointer,
                              block.size,
                              block.scanContents,
                              false});
        }
        std::sort(blocks.begin(), blocks.end(), [](const auto& left, const auto& right) {
            return left.base < right.base;
        });
        std::vector<size_t> work;
        work.reserve(blocks.size() / 4u + 1u);
        auto markCandidate = [&](uintptr_t candidate) {
            auto it = std::upper_bound(blocks.begin(),
                                       blocks.end(),
                                       candidate,
                                       [](uintptr_t value, const NxManagedBlockView& block) {
                                           return value < block.base;
                                       });
            if (it == blocks.begin())
                return;
            --it;
            if (candidate < it->base || candidate - it->base >= it->size || it->marked)
                return;
            it->marked = true;
            const size_t index = static_cast<size_t>(it - blocks.begin());
            if (it->scanContents && it->size <= kNxGcMaxScannedBlock)
                work.push_back(index);
        };
        auto scanRange = [&](uintptr_t begin, uintptr_t end) {
            constexpr uintptr_t wordSize = sizeof(uintptr_t);
            begin = (begin + wordSize - 1u) & ~(wordSize - 1u);
            for (uintptr_t at = begin; at <= end && end - at >= wordSize; at += wordSize) {
                uintptr_t candidate = 0;
                std::memcpy(&candidate, reinterpret_cast<const void*>(at), sizeof(candidate));
                markCandidate(candidate);
            }
        };
        auto scanStackRange = [&](uintptr_t begin, uintptr_t end) {
            constexpr uintptr_t wordSize = sizeof(uintptr_t);
            begin = (begin + wordSize - 1u) & ~(wordSize - 1u);
            for (uintptr_t at = begin; at <= end && end - at >= wordSize; at += wordSize) {
                markCandidate(nx_gc_read_stack_word(at));
            }
        };
        std::jmp_buf registerSnapshot;
        (void)setjmp(registerSnapshot);
        scanStackRange(stackStart, stackEnd);
        scanRange(reinterpret_cast<uintptr_t>(&registerSnapshot),
                  reinterpret_cast<uintptr_t>(&registerSnapshot) + sizeof(registerSnapshot));
        for (size_t next = 0; next < work.size(); ++next) {
            const auto& block = blocks[work[next]];
            scanRange(block.base, block.base + block.size);
        }
        for (const auto& block : blocks) {
            if (!block.marked) {
                auto it = gNxManagedBlocks.find(block.pointer);
                if (it != gNxManagedBlocks.end() && it->second.owner == owner) {
                    std::free(block.pointer);
                    gNxManagedBlocks.erase(it);
                }
            }
        }
        gNxBytesSinceCollection = 0;
    }
#ifdef NEXUS_RUNTIME_TESTING
    static size_t nx_test_live_managed_bytes() {
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        size_t total = 0;
        const auto owner = std::this_thread::get_id();
        for (const auto& [pointer, block] : gNxManagedBlocks) {
            (void)pointer;
            if (block.owner == owner)
                total += block.size;
        }
        return total;
    }
#endif
