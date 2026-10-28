    struct SQLiteApi {

        using Db = void;

        using Open = int (*)(const char*, Db**);

        using Exec = int (*)(Db*, const char*, int (*)(void*, int, char**, char**), void*, char**);

        using Close = int (*)(Db*);

        void* handle = nullptr;
        Open open = nullptr;
        Exec exec = nullptr;
        Close close = nullptr;
    };

    static SQLiteApi gSqlite;

    static std::unordered_map<int64_t, SQLiteApi::Db*> gDatabases;

    static int64_t gNextDb = 1;

    static bool load_sqlite_runtime() {

        if (gSqlite.open)
            return true;

#if defined(_WIN32)
        HMODULE h = LoadLibraryA("sqlite3.dll");

        if (!h)
            return false;

        gSqlite.handle = h;

        gSqlite.open = reinterpret_cast<SQLiteApi::Open>(GetProcAddress(h, "sqlite3_open"));

        gSqlite.exec = reinterpret_cast<SQLiteApi::Exec>(GetProcAddress(h, "sqlite3_exec"));

        gSqlite.close = reinterpret_cast<SQLiteApi::Close>(GetProcAddress(h, "sqlite3_close"));

#else
        void* h = dlopen("libsqlite3.so.0", RTLD_LAZY);

        if (!h)
            h = dlopen("libsqlite3.so", RTLD_LAZY);

        if (!h)
            return false;

        gSqlite.handle = h;

        gSqlite.open = reinterpret_cast<SQLiteApi::Open>(dlsym(h, "sqlite3_open"));

        gSqlite.exec = reinterpret_cast<SQLiteApi::Exec>(dlsym(h, "sqlite3_exec"));

        gSqlite.close = reinterpret_cast<SQLiteApi::Close>(dlsym(h, "sqlite3_close"));

#endif
        return gSqlite.open && gSqlite.exec && gSqlite.close;
    }
