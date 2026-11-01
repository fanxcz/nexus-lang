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
    if (sit == gScenes.end() || !path)
        return false;
    for (const auto& e : sit->second.entities) {
        if (!std::isfinite(e.x) || !std::isfinite(e.y) || !std::isfinite(e.vx) ||
            !std::isfinite(e.vy) || !std::isfinite(e.w) || !std::isfinite(e.h) ||
            !std::isfinite(e.r) || !std::isfinite(e.g) || !std::isfinite(e.b) ||
            !std::isfinite(e.a))
            return false;
    }

    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (!f)
        return false;
    f << std::setprecision(std::numeric_limits<float>::max_digits10);
    f << "NEXUS_SCENE 1\n" << sit->second.entities.size() << "\n";
    for (const auto& e : sit->second.entities) {
        f << e.x << ' ' << e.y << ' ' << e.vx << ' ' << e.vy << ' ' << e.w << ' ' << e.h << ' '
          << e.r << ' ' << e.g << ' ' << e.b << ' ' << e.a << ' ' << e.texture << "\n";
    }
    f.flush();
    return f.good();
}
int64_t nexus_scene_load(const char* path) {
    if (!path)
        return 0;
    std::ifstream f(path);
    if (!f)
        return 0;

    std::string magic;
    int version = 0, count = 0;
    if (!(f >> magic >> version >> count) || magic != "NEXUS_SCENE" ||
        version != 1 || count < 0 || count > 100000)
        return 0;

    std::vector<EcsEntity> entities;
    try {
        entities.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i) {
            EcsEntity e;
            if (!(f >> e.x >> e.y >> e.vx >> e.vy >> e.w >> e.h >> e.r >> e.g >> e.b >> e.a >>
                  e.texture))
                return 0;
            if (!std::isfinite(e.x) || !std::isfinite(e.y) || !std::isfinite(e.vx) ||
                !std::isfinite(e.vy) || !std::isfinite(e.w) || !std::isfinite(e.h) ||
                !std::isfinite(e.r) || !std::isfinite(e.g) || !std::isfinite(e.b) ||
                !std::isfinite(e.a))
                return 0;
            entities.push_back(e);
        }
    } catch (const std::exception&) {
        return 0;
    }

    // A valid snapshot must contain exactly its declared records, followed only
    // by whitespace. Reject appended data rather than silently ignoring it.
    f >> std::ws;
    if (f.peek() != std::char_traits<char>::eof())
        return 0;

    try {
        const int64_t id = nexus_scene_create();
        auto loaded = gScenes.find(id);
        if (loaded == gScenes.end())
            return 0;
        loaded->second.entities = std::move(entities);
        return id;
    } catch (const std::exception&) {
        return 0;
    }
}
// Replace the live ECS registry with the entities stored in a scene snapshot.
// Existing entity handles become invalid; texture handles remain owned by the runtime.
int64_t nexus_scene_apply(int64_t scene) {
    const auto snapshot = gScenes.find(scene);
    if (snapshot == gScenes.end())
        return -1;

    // Build the replacement first. If allocation fails, keep the previous
    // live world intact rather than clearing it halfway through the operation.
    std::unordered_map<int64_t, EcsEntity> replacement;
    try {
        replacement.reserve(snapshot->second.entities.size());
        for (const auto& entity : snapshot->second.entities) {
            if (gNextEntity <= 0 || gNextEntity == std::numeric_limits<int64_t>::max())
                return -1;
            const int64_t id = gNextEntity;
            if (!replacement.emplace(id, entity).second)
                return -1;
            ++gNextEntity;
        }
    } catch (const std::exception&) {
        return -1;
    }

    const int64_t applied = static_cast<int64_t>(replacement.size());
    gEntities.swap(replacement);
    return applied;
}
void nexus_scene_destroy(int64_t id) {
    gScenes.erase(id);
}
void nexus_scene_clear(int64_t id) {
    auto it = gScenes.find(id);
    if (it != gScenes.end())
        it->second.entities.clear();
}
