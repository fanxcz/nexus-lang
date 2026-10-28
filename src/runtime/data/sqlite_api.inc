int64_t nexus_sqlite_open(const char* path) {

    if (!load_sqlite_runtime())
        return 0;
    SQLiteApi::Db* db = nullptr;
    if (gSqlite.open(path ? path : ":memory:", &db) != 0 || !db)
        return 0;
    const int64_t id = gNextDb++;
    gDatabases[id] = db;
    return id;
}
int64_t nexus_sqlite_exec(int64_t id, const char* sql) {

    auto it = gDatabases.find(id);
    if (it == gDatabases.end() || !gSqlite.exec)
        return -1;
    char* err = nullptr;
    const int rc = gSqlite.exec(it->second, sql ? sql : "", nullptr, nullptr, &err);
    if (err) {
        std::fprintf(stderr, "SQLite: %s\n", err);
        if (std::strlen(err))
            std::free(err);
    }
    return rc;
}
void nexus_sqlite_close(int64_t id) {
    auto it = gDatabases.find(id);
    if (it == gDatabases.end())
        return;
    if (gSqlite.close)
        gSqlite.close(it->second);
    gDatabases.erase(it);
}
