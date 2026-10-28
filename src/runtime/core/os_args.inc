
int64_t nexus_os_args_count() {
    auto args = get_os_args();
    return static_cast<int64_t>(args.size());
}

char* nexus_os_args_get(int64_t index) {
    auto args = get_os_args();
    if (index < 0 || static_cast<size_t>(index) >= args.size()) {
        return nx_strdup("");
    }
    return nx_strdup(args[static_cast<size_t>(index)].c_str());
}
