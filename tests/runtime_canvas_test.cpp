#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" int nexus_runtime_test_canvas_blend_pixel(const uint8_t* destination,
                                                       const uint8_t* source,
                                                       uint8_t* result);
extern "C" int nexus_runtime_test_canvas_import_rgba(int64_t destinationWidth,
                                                       int64_t destinationHeight,
                                                       const uint8_t* destinationPixels,
                                                       const uint8_t* sourcePixels,
                                                       int64_t sourceWidth,
                                                       int64_t sourceHeight,
                                                       int64_t sourcePitch,
                                                       uint8_t* resultPixels);
extern "C" int nexus_runtime_test_canvas_blit_rgba(int64_t width,
                                                     int64_t height,
                                                     const uint8_t* destinationPixels,
                                                     const uint8_t* sourcePixels,
                                                     uint8_t* resultPixels);

static bool expectPixel(const uint8_t* actual,
                        const std::array<uint8_t, 4>& expected,
                        const char* label) {
    if (std::memcmp(actual, expected.data(), expected.size()) == 0)
        return true;
    std::fprintf(stderr,
                 "runtime canvas test failed: %s (got %u,%u,%u,%u; expected %u,%u,%u,%u)\n",
                 label,
                 unsigned(actual[0]), unsigned(actual[1]),
                 unsigned(actual[2]), unsigned(actual[3]),
                 unsigned(expected[0]), unsigned(expected[1]),
                 unsigned(expected[2]), unsigned(expected[3]));
    return false;
}

int main() {
    bool ok = true;
    uint8_t result[4]{};

    const std::array<uint8_t, 4> transparent{0, 0, 0, 0};
    const std::array<uint8_t, 4> opaqueBlue{0, 0, 255, 255};
    const std::array<uint8_t, 4> halfRed{255, 0, 0, 128};

    ok &= nexus_runtime_test_canvas_blend_pixel(
              opaqueBlue.data(), transparent.data(), result) != 0;
    ok &= expectPixel(result, opaqueBlue, "transparent source leaves destination unchanged");

    ok &= nexus_runtime_test_canvas_blend_pixel(
              opaqueBlue.data(), halfRed.data(), result) != 0;
    ok &= expectPixel(result, {128, 0, 127, 255}, "half-transparent red over opaque blue");

    ok &= nexus_runtime_test_canvas_blend_pixel(
              transparent.data(), halfRed.data(), result) != 0;
    ok &= expectPixel(result, {255, 0, 0, 128}, "half-transparent source over transparent pixel");

    const std::array<uint8_t, 4> opaqueGreen{0, 255, 0, 255};
    ok &= nexus_runtime_test_canvas_blend_pixel(
              opaqueBlue.data(), opaqueGreen.data(), result) != 0;
    ok &= expectPixel(result, opaqueGreen, "opaque source replaces destination");

    // Import a 2x1 image into a 4x4 transparent canvas. The image must be
    // scaled to 4x2, centered vertically, and preserve its two source colors.
    const std::vector<uint8_t> destination(4u * 4u * 4u, 0);
    const std::array<uint8_t, 8> source{
        255, 0, 0, 255, 0, 0, 255, 255
    };
    std::vector<uint8_t> imported(destination.size(), 0);
    ok &= nexus_runtime_test_canvas_import_rgba(
              4, 4, destination.data(), source.data(), 2, 1, 8, imported.data()) != 0;
    ok &= expectPixel(imported.data(), transparent, "fit leaves top margin transparent");
    ok &= expectPixel(imported.data() + 1u * 4u * 4u,
                      {255, 0, 0, 255}, "fit centers first source pixel");
    ok &= expectPixel(imported.data() + (1u * 4u + 1u) * 4u,
                      {255, 0, 0, 255}, "nearest-neighbor scale repeats first pixel");
    ok &= expectPixel(imported.data() + (1u * 4u + 2u) * 4u,
                      {0, 0, 255, 255}, "nearest-neighbor scale maps second pixel");
    ok &= expectPixel(imported.data() + (2u * 4u + 3u) * 4u,
                      {0, 0, 255, 255}, "fitted image reaches right edge");
    ok &= expectPixel(imported.data() + 3u * 4u * 4u,
                      transparent, "fit leaves bottom margin transparent");

    // Imported alpha is composited instead of overwriting the destination.
    const std::array<uint8_t, 4> white{255, 255, 255, 255};
    const std::array<uint8_t, 4> halfGreen{0, 255, 0, 128};
    std::array<uint8_t, 4> whiteDestination{};
    std::array<uint8_t, 4> greenResult{};
    ok &= nexus_runtime_test_canvas_import_rgba(
              1, 1, white.data(), halfGreen.data(), 1, 1, 4, greenResult.data()) != 0;
    ok &= expectPixel(greenResult.data(), {127, 255, 127, 255},
                      "image import composites source alpha over canvas");

    // Canvas blit uses source-over alpha and rejects invalid dimensions.
    const std::array<uint8_t, 8> destinationRow{
        0, 0, 255, 255, 10, 20, 30, 255
    };
    const std::array<uint8_t, 8> sourceRow{
        255, 0, 0, 128, 0, 0, 0, 0
    };
    std::array<uint8_t, 8> blitResult{};
    ok &= nexus_runtime_test_canvas_blit_rgba(
              2, 1, destinationRow.data(), sourceRow.data(), blitResult.data()) != 0;
    ok &= expectPixel(blitResult.data(), {128, 0, 127, 255},
                      "canvas blit composites source pixel");
    ok &= expectPixel(blitResult.data() + 4, {10, 20, 30, 255},
                      "canvas blit preserves transparent source pixel");

    ok &= nexus_runtime_test_canvas_import_rgba(
              4, 4, destination.data(), source.data(), 2, 1, 7, imported.data()) == 0;
    ok &= nexus_runtime_test_canvas_import_rgba(
              0, 4, destination.data(), source.data(), 2, 1, 8, imported.data()) == 0;
    ok &= nexus_runtime_test_canvas_blit_rgba(
              2, 1, destinationRow.data(), sourceRow.data(), blitResult.data()) != 0;

    return ok ? 0 : 1;
}
