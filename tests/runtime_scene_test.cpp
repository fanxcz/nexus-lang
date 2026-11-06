#include <cstdint>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

extern "C" int64_t nexus_scene_create();
extern "C" bool nexus_scene_save(int64_t scene, const char* path);
extern "C" int64_t nexus_scene_load(const char* path);
extern "C" void nexus_scene_destroy(int64_t scene);
extern "C" int64_t nexus_ecs_create();
extern "C" void nexus_ecs_destroy(int64_t id);
extern "C" void nexus_ecs_set_position(int64_t id, double x, double y);
extern "C" void nexus_ecs_set_velocity(int64_t id, double x, double y);
extern "C" void nexus_ecs_set_size(int64_t id, double w, double h);
extern "C" void nexus_scene_add(int64_t scene, int64_t entity);

namespace fs = std::filesystem;

static std::string readAll(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory = fs::temp_directory_path() /
                               ("nexus-scene-save-test-" + std::to_string(stamp));
    std::error_code ec;
    fs::create_directories(directory, ec);
    if (ec)
        return 1;

    const fs::path destination = directory / "scene.nxs";
    const fs::path roundTrip = directory / "round-trip.nxs";
    const int64_t scene = nexus_scene_create();
    const int64_t entity = nexus_ecs_create();
    nexus_ecs_set_position(entity, 12.5, -3.25);
    nexus_ecs_set_velocity(entity, 4.0, 8.0);
    nexus_ecs_set_size(entity, 64.0, 32.0);
    nexus_scene_add(scene, entity);

    int result = 0;
    {
        std::ofstream old(destination, std::ios::binary);
        old << "previous save";
        if (!old)
            result = 2;
    }
    if (result == 0 && !nexus_scene_save(scene, destination.string().c_str()))
        result = 3;

    const std::string saved = readAll(destination);
    if (result == 0 && saved.find("NEXUS_SCENE 1\n1\n") != 0)
        result = 4;

    // A directory cannot be replaced by a scene file. The failed save must
    // leave the directory intact and clean up its temporary sibling.
    if (result == 0 && nexus_scene_save(scene, directory.string().c_str()))
        result = 5;
    if (result == 0 && !fs::is_directory(directory))
        result = 6;
    if (result == 0) {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().filename().string().find(".nexus-tmp-") != std::string::npos) {
                result = 7;
                break;
            }
        }
    }

    const int64_t loaded = result == 0 ? nexus_scene_load(destination.string().c_str()) : 0;
    if (result == 0 && loaded == 0)
        result = 8;
    if (result == 0 && !nexus_scene_save(loaded, roundTrip.string().c_str()))
        result = 9;
    if (result == 0 && readAll(roundTrip) != saved)
        result = 10;

    // Invalid scene handles must not modify an existing good save.
    if (result == 0 && nexus_scene_save(-1, destination.string().c_str()))
        result = 11;
    if (result == 0 && readAll(destination) != saved)
        result = 12;

    nexus_scene_destroy(scene);
    if (loaded != 0)
        nexus_scene_destroy(loaded);
    nexus_ecs_destroy(entity);
    fs::remove_all(directory, ec);
    if (result == 0 && ec)
        result = 13;

    if (result != 0) {
        std::fprintf(stderr, "NEXUS scene save test failed at check %d\n", result);
        return result;
    }
    std::puts("NEXUS scene save smoke: PASS");
    return 0;
}
