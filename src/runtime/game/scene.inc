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
    std::ofstream f(path);
    if (!f)
        return false;
    f << "NEXUS_SCENE 1\n" << sit->second.entities.size() << "\n";
    for (const auto& e : sit->second.entities) {
        f << e.x << ' ' << e.y << ' ' << e.vx << ' ' << e.vy << ' ' << e.w << ' ' << e.h << ' '
          << e.r << ' ' << e.g << ' ' << e.b << ' ' << e.a << ' ' << e.texture << "\n";
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
