// Verify that graphics and audio entry points stay safe when no backend has
// been initialized. This covers headless CI machines and systems where the SDL
// runtime libraries are missing: callers must get documented sentinel values
// and an error string instead of a crash.
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" const char* nexus_gfx_backend();
extern "C" const char* nexus_gfx_error();
extern "C" const char* nexus_gfx_audio_error();
extern "C" int64_t nexus_gfx_audio_available();
extern "C" bool nexus_gfx_clipboard_set(const char* text);
extern "C" char* nexus_gfx_clipboard_get();
extern "C" void nexus_free(void* pointer);
extern "C" int64_t nexus_gfx_width();
extern "C" int64_t nexus_gfx_height();
extern "C" int64_t nexus_gfx_sound_load(const char* path);
extern "C" bool nexus_gfx_vsync(bool enabled);
extern "C" void nexus_gfx_clear(double r, double g, double b, double a);
extern "C" void
nexus_gfx_rect(double x, double y, double w, double h, double r, double g, double b, double a);
extern "C" void nexus_gfx_end();
extern "C" void nexus_gfx_shutdown();

static int failures = 0;

static void expect(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "backend state check failed: %s\n", message);
        ++failures;
    }
}

int main() {
    // Before nexus_gfx_init the runtime reports an explicit uninitialized
    // backend, keeps its configured logical size, and offers no vsync.
    expect(std::strcmp(nexus_gfx_backend(), "uninitialized") == 0,
           "backend name must be 'uninitialized' before init");
    expect(nexus_gfx_width() == 1280 && nexus_gfx_height() == 720,
           "logical size defaults must be 1280x720 before init");
    expect(!nexus_gfx_vsync(true), "vsync must be unavailable before init");

    // SDL clipboard access is optional and requires an initialized video
    // backend. Headless callers receive documented sentinels, not a crash.
    expect(!nexus_gfx_clipboard_set("NEXUS"), "clipboard write must fail before graphics init");
    char* clipboard = nexus_gfx_clipboard_get();
    expect(clipboard != nullptr && clipboard[0] == '\0',
           "clipboard read must return an empty string before graphics init");
    if (clipboard)
        nexus_free(clipboard);
    const char* clipboardError = nexus_gfx_error();
    expect(clipboardError && std::strstr(clipboardError, "clipboard") != nullptr,
           "clipboard access without graphics init must record a reason");

    // Drawing calls are documented no-ops before initialization. They must not
    // dereference a null window or renderer.
    nexus_gfx_clear(0.0, 0.0, 0.0, 1.0);
    nexus_gfx_rect(0.0, 0.0, 10.0, 10.0, 1.0, 1.0, 1.0, 1.0);
    nexus_gfx_end();
    nexus_gfx_shutdown();
    nexus_gfx_shutdown(); // Shutdown is idempotent.

    expect(std::strcmp(nexus_gfx_backend(), "uninitialized") == 0,
           "backend must remain uninitialized after shutdown without init");

    // A missing sound file must return the 0 sentinel and record a reason. This
    // also exercises the path where SDL audio is unavailable or has no device.
    const int64_t sound = nexus_gfx_sound_load("nexus-definitely-missing.wav");
    expect(sound == 0, "missing sound file must return 0");
    expect(nexus_gfx_audio_available() == 0, "audio must be unavailable without a device");
    const char* audioError = nexus_gfx_audio_error();
    expect(audioError != nullptr && audioError[0] != '\0',
           "missing sound file must record an audio error");

    const char* graphicsError = nexus_gfx_error();
    expect(graphicsError != nullptr, "graphics error accessor must never return null");

    if (failures != 0)
        return 1;
    std::printf("NEXUS backend state smoke: PASS\n");
    return 0;
}
