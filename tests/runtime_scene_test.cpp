#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
int64_t nexus_scene_create();
void nexus_scene_add(int64_t scene, int64_t entity);
bool nexus_scene_save(int64_t scene, const char* path);
int64_t nexus_scene_load(const char* path);
int64_t nexus_scene_apply(int64_t scene);
void nexus_scene_destroy(int64_t scene);

int64_t nexus_ecs_create();
void nexus_ecs_set_position(int64_t id, double x, double y);
bool nexus_ecs_alive(int64_t id);
int64_t nexus_ecs_count();
}

int main() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = (std::filesystem::temp_directory_path() /
                       ("nexus-scene-roundtrip-" + std::to_string(nonce) + ".nxs")).string();

    const int64_t snapshot = nexus_scene_create();
    const int64_t first = nexus_ecs_create();
    const int64_t second = nexus_ecs_create();
    nexus_ecs_set_position(first, 12.5, 24.5);
    nexus_ecs_set_position(second, 90.0, 120.0);
    nexus_scene_add(snapshot, first);
    nexus_scene_add(snapshot, second);

    if (!nexus_scene_save(snapshot, path.c_str())) {
        std::fprintf(stderr, "scene snapshot save failed\n");
        return 1;
    }

    const auto missingDirectory = std::filesystem::temp_directory_path() /
                                  ("nexus-scene-missing-" + std::to_string(nonce));
    std::filesystem::remove_all(missingDirectory);
    const auto unwritablePath = (missingDirectory / "scene.nxs").string();
    if (nexus_scene_save(snapshot, unwritablePath.c_str())) {
        std::fprintf(stderr, "scene snapshot save unexpectedly succeeded for a missing directory\n");
        std::filesystem::remove(path);
        return 10;
    }

    const auto expectRejected = [&](const std::string& name, const std::string& payload) {
        const auto invalidPath = (std::filesystem::temp_directory_path() /
                                  ("nexus-scene-invalid-" + name + "-" +
                                   std::to_string(nonce) + ".nxs")).string();
        {
            std::ofstream invalid(invalidPath, std::ios::out | std::ios::trunc);
            if (!invalid)
                return false;
            invalid << payload;
            if (!invalid.good())
                return false;
        }
        const int64_t before = nexus_ecs_count();
        const int64_t loadedInvalid = nexus_scene_load(invalidPath.c_str());
        std::error_code error;
        std::filesystem::remove(invalidPath, error);
        return loadedInvalid == 0 && nexus_ecs_count() == before && !error;
    };
    if (!expectRejected("truncated", "NEXUS_SCENE 1\n1\n1 2 3\n") ||
        !expectRejected("nonfinite", "NEXUS_SCENE 1\n1\nnan 0 0 0 1 1 1 1 1 1 0\n") ||
        !expectRejected("trailing", "NEXUS_SCENE 1\n0\nextra data\n") ||
        !expectRejected("oversized", "NEXUS_SCENE 1\n100001\n")) {
        std::fprintf(stderr, "malformed scene snapshots were not rejected safely\n");
        std::filesystem::remove(path);
        return 11;
    }

    const int64_t extra = nexus_ecs_create();
    if (nexus_ecs_count() != 3) {
        std::fprintf(stderr, "unexpected live ECS count before scene apply\n");
        return 2;
    }

    const int64_t loaded = nexus_scene_load(path.c_str());
    if (!loaded) {
        std::fprintf(stderr, "scene snapshot load failed\n");
        return 3;
    }

    const int64_t applied = nexus_scene_apply(loaded);
    if (applied != 2 || nexus_ecs_count() != 2) {
        std::fprintf(stderr, "scene apply did not restore the two saved entities: applied=%lld count=%lld\n",
                     static_cast<long long>(applied),
                     static_cast<long long>(nexus_ecs_count()));
        return 4;
    }
    if (nexus_ecs_alive(first) || nexus_ecs_alive(second) || nexus_ecs_alive(extra)) {
        std::fprintf(stderr, "scene apply did not invalidate pre-apply entity handles\n");
        return 5;
    }

    const int64_t beforeInvalidApply = nexus_ecs_count();
    if (nexus_scene_apply(0) != -1 || nexus_ecs_count() != beforeInvalidApply) {
        std::fprintf(stderr, "invalid scene handle modified the live ECS registry\n");
        return 6;
    }

    const int64_t empty = nexus_scene_create();
    if (nexus_scene_apply(empty) != 0 || nexus_ecs_count() != 0) {
        std::fprintf(stderr, "applying an empty snapshot did not clear the live ECS registry\n");
        return 7;
    }

    if (nexus_scene_apply(loaded) != 2 || nexus_ecs_count() != 2) {
        std::fprintf(stderr, "a loaded snapshot could not be applied more than once\n");
        return 8;
    }

    nexus_scene_destroy(empty);
    nexus_scene_destroy(loaded);
    nexus_scene_destroy(snapshot);
    std::error_code removeError;
    std::filesystem::remove(path, removeError);
    if (removeError) {
        std::fprintf(stderr, "could not remove temporary scene snapshot: %s\n", removeError.message().c_str());
        return 9;
    }

    std::puts("NEXUS runtime scene round-trip: PASS");
    return 0;
}
