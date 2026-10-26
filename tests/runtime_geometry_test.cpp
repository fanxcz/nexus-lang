#include <cstdio>

extern "C" bool nexus_physics_aabb(double ax, double ay, double aw, double ah,
                                   double bx, double by, double bw, double bh);
extern "C" bool nexus_physics_circle(double ax, double ay, double ar,
                                     double bx, double by, double br);

static bool expect(bool actual, bool wanted, const char* name) {
    if (actual == wanted)
        return true;
    std::fprintf(stderr, "runtime geometry check failed: %s (got %d, expected %d)\n",
                 name, actual ? 1 : 0, wanted ? 1 : 0);
    return false;
}

int main() {
    bool ok = true;
    ok &= expect(nexus_physics_aabb(0, 0, 10, 10, 5, 5, 10, 10), true, "AABB overlap");
    ok &= expect(nexus_physics_aabb(0, 0, 10, 10, 10, 0, 5, 5), false, "AABB touching edges");
    ok &= expect(nexus_physics_aabb(0, 0, 10, 10, 20, 20, 5, 5), false, "AABB separated");
    ok &= expect(nexus_physics_aabb(0, 0, 0, 10, 0, 0, 4, 4), false, "AABB zero width");
    ok &= expect(nexus_physics_aabb(0, 0, -1, 10, 0, 0, 4, 4), false, "AABB negative width");
    ok &= expect(nexus_physics_aabb(0, 0, 10, 10, 0, 0, 4, -2), false, "AABB negative height");

    ok &= expect(nexus_physics_circle(0, 0, 5, 6, 0, 2), true, "circle overlap");
    ok &= expect(nexus_physics_circle(0, 0, 5, 7, 0, 2), true, "circle tangent");
    ok &= expect(nexus_physics_circle(0, 0, 5, 7.1, 0, 2), false, "circles separated");
    ok &= expect(nexus_physics_circle(0, 0, -1, 0, 0, 2), false, "negative first radius");
    ok &= expect(nexus_physics_circle(0, 0, 1, 0, 0, -2), false, "negative second radius");
    return ok ? 0 : 1;
}
