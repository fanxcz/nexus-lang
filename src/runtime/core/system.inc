int64_t nexus_system(const char* c) {
    return c ? std::system(c) : -1;
}
