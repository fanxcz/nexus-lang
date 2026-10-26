#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <limits>

extern "C" char* nexus_ui_text_field(double x, double y, double w, double h,
                                      const char* current, const char* placeholder, int64_t maxBytes);
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

    // A non-interactive field should borrow an unchanged input string rather
    // than allocate a new C string every frame.
    const char* stableText = "stable value";
    char* unchangedText = nexus_ui_text_field(0.0, 0.0, 100.0, 30.0,
                                               stableText, "placeholder", 64);
    ok &= expect(unchangedText == stableText, true, "text field reuses unchanged string");

    // Truncation is a real edit and must still return the bounded UTF-8 value.
    const char* longText = "hello";
    char* shortenedText = nexus_ui_text_field(0.0, 0.0, 100.0, 30.0,
                                                longText, "placeholder", 3);
    ok &= expect(std::strcmp(shortenedText, "hel") == 0, true, "text field truncates to byte limit");

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

    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double largest = std::numeric_limits<double>::max();
    ok &= expect(nexus_physics_aabb(0, 0, infinity, 10, 1, 1, 2, 2),
                 false, "AABB rejects infinite dimensions");
    ok &= expect(nexus_physics_aabb(nan, 0, 10, 10, 1, 1, 2, 2),
                 false, "AABB rejects NaN coordinates");
    ok &= expect(nexus_physics_aabb(largest, 0, largest, 10, 0, 0, 1, 1),
                 false, "AABB rejects overflowed edges");
    ok &= expect(nexus_physics_circle(0, 0, infinity, 1, 0, 1),
                 false, "circle rejects infinite radius");
    ok &= expect(nexus_physics_circle(nan, 0, 1, 0, 0, 1),
                 false, "circle rejects NaN coordinates");
    ok &= expect(nexus_physics_circle(largest, 0, 1, -largest, 0, 1),
                 false, "circle rejects overflowed center distance");
    ok &= expect(nexus_physics_circle(0, 0, largest, 1, 0, largest),
                 false, "circle rejects overflowed radius sum");
    return ok ? 0 : 1;
}
