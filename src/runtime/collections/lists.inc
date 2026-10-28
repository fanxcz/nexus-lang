// exits, matching the managed-string lifetime model used elsewhere.
static std::unordered_map<int64_t, std::vector<int64_t>> gILists;
static std::unordered_map<int64_t, std::vector<std::string>> gSLists;
static int64_t gNextListId = 1;

int64_t nexus_ilist_new() {
    int64_t id = gNextListId++;
    gILists.emplace(id, std::vector<int64_t>{});
    return id;
}
void nexus_ilist_push(int64_t h, int64_t v) {
    auto it = gILists.find(h);
    if (it != gILists.end())
        it->second.push_back(v);
}
int64_t nexus_ilist_len(int64_t h) {
    auto it = gILists.find(h);
    return it == gILists.end() ? 0 : static_cast<int64_t>(it->second.size());
}
int64_t nexus_ilist_get(int64_t h, int64_t i) {
    auto it = gILists.find(h);
    if (it == gILists.end() || i < 0 || static_cast<size_t>(i) >= it->second.size())
        return 0;
    return it->second[static_cast<size_t>(i)];
}
void nexus_ilist_set(int64_t h, int64_t i, int64_t v) {
    auto it = gILists.find(h);
    if (it != gILists.end() && i >= 0 && static_cast<size_t>(i) < it->second.size())
        it->second[static_cast<size_t>(i)] = v;
}
int64_t nexus_slist_new() {
    int64_t id = gNextListId++;
    gSLists.emplace(id, std::vector<std::string>{});
    return id;
}
void nexus_slist_push(int64_t h, const char* s) {
    auto it = gSLists.find(h);
    if (it != gSLists.end())
        it->second.emplace_back(s ? s : "");
}
int64_t nexus_slist_len(int64_t h) {
    auto it = gSLists.find(h);
    return it == gSLists.end() ? 0 : static_cast<int64_t>(it->second.size());
}
char* nexus_slist_get(int64_t h, int64_t i) {
    auto it = gSLists.find(h);
    if (it == gSLists.end() || i < 0 || static_cast<size_t>(i) >= it->second.size())
        return nx_strdup("");
    return nx_strdup(it->second[static_cast<size_t>(i)].c_str());
}

// Checked collection operations report invalid handles, indexes and allocation
// failures instead of silently dropping mutations or conflating them with values.
bool nexus_ilist_push_checked(int64_t h, int64_t value) {
    auto it = gILists.find(h);
    if (it == gILists.end())
        return false;
    try {
        it->second.push_back(value);
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_ilist_reserve(int64_t h, int64_t capacity) {
    auto it = gILists.find(h);
    if (it == gILists.end() || capacity < 0)
        return false;
    try {
        it->second.reserve(static_cast<size_t>(capacity));
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_ilist_clear(int64_t h) {
    auto it = gILists.find(h);
    if (it == gILists.end())
        return false;
    it->second.clear();
    return true;
}
bool nexus_ilist_free(int64_t h) {
    return gILists.erase(h) != 0;
}
bool nexus_ilist_insert(int64_t h, int64_t index, int64_t value) {
    auto it = gILists.find(h);
    if (it == gILists.end() || index < 0 ||
        static_cast<unsigned long long>(index) > it->second.size())
        return false;
    try {
        it->second.insert(it->second.begin() + static_cast<size_t>(index), value);
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_ilist_try_get(int64_t h, int64_t index, int64_t* out) {
    auto it = gILists.find(h);
    if (!out || it == gILists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    *out = it->second[static_cast<size_t>(index)];
    return true;
}
bool nexus_ilist_set_checked(int64_t h, int64_t index, int64_t value) {
    auto it = gILists.find(h);
    if (it == gILists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    it->second[static_cast<size_t>(index)] = value;
    return true;
}
bool nexus_ilist_remove(int64_t h, int64_t index, int64_t* out) {
    auto it = gILists.find(h);
    if (!out || it == gILists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    auto& values = it->second;
    *out = values[static_cast<size_t>(index)];
    values.erase(values.begin() + static_cast<size_t>(index));
    return true;
}
bool nexus_ilist_try_pop(int64_t h, int64_t* out) {
    auto it = gILists.find(h);
    if (!out || it == gILists.end() || it->second.empty())
        return false;
    *out = it->second.back();
    it->second.pop_back();
    return true;
}

bool nexus_slist_push_checked(int64_t h, const char* value) {
    auto it = gSLists.find(h);
    if (it == gSLists.end())
        return false;
    try {
        it->second.emplace_back(value ? value : "");
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_slist_reserve(int64_t h, int64_t capacity) {
    auto it = gSLists.find(h);
    if (it == gSLists.end() || capacity < 0)
        return false;
    try {
        it->second.reserve(static_cast<size_t>(capacity));
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_slist_clear(int64_t h) {
    auto it = gSLists.find(h);
    if (it == gSLists.end())
        return false;
    it->second.clear();
    return true;
}
bool nexus_slist_free(int64_t h) {
    return gSLists.erase(h) != 0;
}
bool nexus_slist_insert(int64_t h, int64_t index, const char* value) {
    auto it = gSLists.find(h);
    if (it == gSLists.end() || index < 0 ||
        static_cast<unsigned long long>(index) > it->second.size())
        return false;
    try {
        it->second.insert(it->second.begin() + static_cast<size_t>(index), value ? value : "");
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_slist_try_get(int64_t h, int64_t index, char** out) {
    auto it = gSLists.find(h);
    if (!out || it == gSLists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    char* copy = nx_strdup(it->second[static_cast<size_t>(index)].c_str());
    if (!copy)
        return false;
    *out = copy;
    return true;
}
bool nexus_slist_set_checked(int64_t h, int64_t index, const char* value) {
    auto it = gSLists.find(h);
    if (it == gSLists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    try {
        it->second[static_cast<size_t>(index)] = value ? value : "";
        return true;
    } catch (...) {
        return false;
    }
}
bool nexus_slist_remove(int64_t h, int64_t index, char** out) {
    auto it = gSLists.find(h);
    if (!out || it == gSLists.end() || index < 0 ||
        static_cast<size_t>(index) >= it->second.size())
        return false;
    auto& values = it->second;
    char* copy = nx_strdup(values[static_cast<size_t>(index)].c_str());
    if (!copy)
        return false;
    values.erase(values.begin() + static_cast<size_t>(index));
    *out = copy;
    return true;
}
bool nexus_slist_try_pop(int64_t h, char** out) {
    auto it = gSLists.find(h);
    if (!out || it == gSLists.end() || it->second.empty())
        return false;
    char* copy = nx_strdup(it->second.back().c_str());
    if (!copy)
        return false;
    it->second.pop_back();
    *out = copy;
    return true;
}
