int64_t nexus_scene_create() {
    int64_t id = gNextScene++;
    gScenes[id] = SceneData{};
    return id;
}
void nexus_scene_add(int64_t scene, int64_t entity) {
    auto sit = gScenes.find(scene);
    auto eit = gEntities.find(entity);
    if (sit != gScenes.end() && eit != gEntities.end())
        sit->second.entities.push_back(eit->second);
}
bool nexus_scene_save(int64_t scene, const char* path) {
    auto sit = gScenes.find(scene);
    if (sit == gScenes.end() || !path || path[0] == '\0')
        return false;

    // Stage the complete file beside the destination. A failed write must not
    // truncate a previously saved scene.
    static std::atomic<uint64_t> temporarySequence{0};
    const auto sequence = temporarySequence.fetch_add(1, std::memory_order_relaxed);
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path destination(path);
    std::filesystem::path temporary = destination;
    temporary += ".nexus-tmp-" + std::to_string(stamp) + "-" + std::to_string(sequence);

    // Scene files use stable LF line endings on every host platform.
    std::ofstream f(temporary, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!f)
        return false;
    f << "NEXUS_SCENE 1\n" << sit->second.entities.size() << "\n";
    for (const auto& e : sit->second.entities) {
        f << e.x << ' ' << e.y << ' ' << e.vx << ' ' << e.vy << ' ' << e.w << ' ' << e.h << ' '
          << e.r << ' ' << e.g << ' ' << e.b << ' ' << e.a << ' ' << e.texture << "\n";
    }
    f.flush();
    const bool written = f.good();
    f.close();
    if (!written || f.fail()) {
        std::error_code cleanupError;
        std::filesystem::remove(temporary, cleanupError);
        return false;
    }

#if defined(_WIN32)
    const bool replaced =
        MoveFileExW(temporary.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code renameError;
    std::filesystem::rename(temporary, destination, renameError);
    const bool replaced = !renameError;
#endif
    if (!replaced) {
        std::error_code cleanupError;
        std::filesystem::remove(temporary, cleanupError);
        return false;
    }
    return true;
}
int64_t nexus_scene_load(const char* path) {
    if (!path)
        return 0;
    std::ifstream f(path);
    if (!f)
        return 0;
    std::string magic;
    int version = 0, count = 0;
    f >> magic >> version >> count;
    if (magic != "NEXUS_SCENE" || version != 1 || count < 0 || count > 100000)
        return 0;
    int64_t sid = nexus_scene_create();
    auto& out = gScenes[sid].entities;
    for (int i = 0; i < count; ++i) {
        EcsEntity e;
        if (!(f >> e.x >> e.y >> e.vx >> e.vy >> e.w >> e.h >> e.r >> e.g >> e.b >> e.a >>
              e.texture)) {
            gScenes.erase(sid);
            return 0;
        }
        out.push_back(e);
    }
    return sid;
}
void nexus_scene_destroy(int64_t id) {
    gScenes.erase(id);
}
void nexus_scene_clear(int64_t id) {
    auto it = gScenes.find(id);
    if (it != gScenes.end())
        it->second.entities.clear();
}
