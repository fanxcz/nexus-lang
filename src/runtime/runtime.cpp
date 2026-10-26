#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <csetjmp>
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
#if defined(_WIN32)
#include <winternl.h>
#endif
#if defined(__APPLE__) || defined(__linux__)
#include <pthread.h>
#endif
#if defined(__APPLE__)
#include <crt_externs.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#else
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

// NEXUS runtime is deliberately built without SDL/OpenGL development headers.
// The graphics backend loads SDL2 at runtime and obtains OpenGL entry points
// from SDL. This keeps the generated NEXUS application portable across hosts
// that provide SDL2/OpenGL without requiring a compiler-time SDK.

namespace {

    using byte = unsigned char;

    typedef struct SDL_Window SDL_Window;
    typedef struct SDL_Renderer SDL_Renderer;
    typedef struct SDL_Texture SDL_Texture;

    typedef void* SDL_GLContext;

    typedef uint32_t SDL_AudioDeviceID;

    typedef uint16_t SDL_AudioFormat;

    typedef uint8_t SDL_AudioStatus;

    typedef uint32_t SDL_AudioCallbackUserdata;

    struct SDL_AudioSpec {

        int freq;

        SDL_AudioFormat format;

        uint8_t channels;

        uint8_t silence;

        uint16_t samples;

        uint16_t padding;

        uint32_t size;

        void (*callback)(void*, uint8_t*, int);

        void* userdata;
    };

    struct SDL_PixelFormat {

        uint32_t format;

        void* palette;

        uint8_t BitsPerPixel;

        uint8_t BytesPerPixel;

        uint8_t padding[2];

        uint32_t Rmask, Gmask, Bmask, Amask;

        uint8_t Rloss, Gloss, Bloss, Aloss;

        uint8_t Rshift, Gshift, Bshift, Ashift;

        int32_t refcount;

        void* next;
    };

    struct SDL_Surface {

        uint32_t flags;

        SDL_PixelFormat* format;

        int w, h;

        int pitch;

        void* pixels;

        void* userdata;

        int locked;

        void* lock_data;

        std::array<int, 2> clip_rect;

        void* map;

        int refcount;

        void* reserved;
    };

    struct SDL_Keysym {

        int32_t scancode;

        int32_t sym;

        uint16_t mod;

        uint32_t unused;
    };

    struct SDL_KeyboardEvent {

        uint32_t type;

        uint32_t timestamp;

        uint32_t windowID;

        uint8_t state;

        uint8_t repeat;

        uint8_t padding2;

        uint8_t padding3;

        SDL_Keysym keysym;
    };

    struct SDL_MouseMotionEvent {

        uint32_t type, timestamp, windowID;

        uint32_t which;

        uint32_t state;

        int x, y, xrel, yrel;
    };

    struct SDL_MouseButtonEvent {

        uint32_t type, timestamp, windowID;

        uint32_t which;

        uint8_t button, state, clicks, padding1;

        int x, y;
    };

    struct SDL_MouseWheelEvent {

        uint32_t type, timestamp, windowID;

        uint32_t which;

        int x, y;

        uint32_t direction;

        double preciseX, preciseY;

        int32_t mouseX, mouseY;
    };

    struct SDL_WindowEvent {

        uint32_t type, timestamp, windowID;

        uint8_t event;

        uint8_t padding1, padding2, padding3;

        int data1, data2;
    };

    struct SDL_TextInputEvent {
        uint32_t type, timestamp, windowID;
        char text[32];
    };

    union SDL_Event {

        uint32_t type;

        uint8_t padding[128];

        SDL_KeyboardEvent key;

        SDL_MouseMotionEvent motion;

        SDL_MouseButtonEvent button;

        SDL_MouseWheelEvent wheel;

        SDL_WindowEvent window;

        SDL_TextInputEvent text;
    };

    struct SDL_Rect {
        int x;
        int y;
        int w;
        int h;
    };

    struct SDL_Color {
        uint8_t r, g, b, a;
    };

    typedef struct TTF_Font TTF_Font;

    typedef struct Mix_Music Mix_Music;

    // SDL constants used by the runtime.
    constexpr uint32_t SDL_INIT_TIMER = 0x00000001u;

    constexpr uint32_t SDL_INIT_AUDIO = 0x00000010u;

    constexpr uint32_t SDL_INIT_VIDEO = 0x00000020u;

    constexpr uint32_t SDL_WINDOW_OPENGL = 0x00000002u;

    constexpr uint32_t SDL_WINDOW_SHOWN = 0x00000004u;

    constexpr uint32_t SDL_WINDOW_RESIZABLE = 0x00000020u;
    constexpr uint32_t SDL_WINDOW_FULLSCREEN_DESKTOP = 0x00001001u;
    constexpr uint32_t SDL_RENDERER_SOFTWARE = 0x00000001u;
    constexpr uint32_t SDL_RENDERER_ACCELERATED = 0x00000002u;
    constexpr uint32_t SDL_RENDERER_PRESENTVSYNC = 0x00000004u;
    constexpr int SDL_TEXTUREACCESS_STATIC = 0;
    constexpr int SDL_TEXTUREACCESS_STREAMING = 1;
    constexpr int SDL_BLENDMODE_NONE = 0;
    constexpr int SDL_BLENDMODE_BLEND = 1;

    constexpr int SDL_WINDOWPOS_CENTERED = 0x2FFF0000;

    constexpr int SDL_GL_CONTEXT_MAJOR_VERSION = 17;

    constexpr int SDL_GL_CONTEXT_MINOR_VERSION = 18;

    constexpr int SDL_GL_CONTEXT_PROFILE_MASK = 21;

    constexpr int SDL_GL_CONTEXT_PROFILE_COMPATIBILITY = 0x0002;
    constexpr int SDL_GL_DOUBLEBUFFER = 5;

    constexpr uint32_t SDL_QUIT = 0x100u;

    constexpr uint32_t SDL_WINDOWEVENT = 0x200u;

    constexpr uint32_t SDL_KEYDOWN = 0x300u;

    constexpr uint32_t SDL_KEYUP = 0x301u;

    constexpr uint32_t SDL_TEXTINPUT = 0x303u;

    constexpr uint32_t SDL_MOUSEMOTION = 0x400u;

    constexpr uint32_t SDL_MOUSEBUTTONDOWN = 0x401u;

    constexpr uint32_t SDL_MOUSEBUTTONUP = 0x402u;

    constexpr uint8_t SDL_PRESSED = 1;

    constexpr uint8_t SDL_RELEASED = 0;

    constexpr uint32_t SDL_BUTTON_LEFT = 1u;

    constexpr uint32_t SDL_BUTTON_MIDDLE = 2u;

    constexpr uint32_t SDL_BUTTON_RIGHT = 3u;

    constexpr uint32_t SDL_PIXELFORMAT_ABGR8888 = 376840196u;

    constexpr uint16_t AUDIO_S16LSB = 0x8010u;

    // OpenGL constants/types.
    using GLenum = unsigned int;

    using GLboolean = unsigned char;

    using GLbitfield = unsigned int;

    using GLvoid = void;

    using GLbyte = signed char;

    using GLshort = short;

    using GLint = int;

    using GLsizei = int;

    using GLuint = unsigned int;

    using GLfloat = float;

    using GLclampf = float;

    using GLdouble = double;

    using GLubyte = unsigned char;

    using GLushort = unsigned short;

    constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;

    constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x00000100;

    constexpr GLenum GL_DEPTH_TEST = 0x0B71;

    constexpr GLenum GL_BLEND = 0x0BE2;

    constexpr GLenum GL_SRC_ALPHA = 0x0302;

    constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;

    constexpr GLenum GL_TEXTURE_2D = 0x0DE1;

    constexpr GLenum GL_RGBA = 0x1908;

    constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;

    constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;

    constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;

    constexpr GLenum GL_LINEAR = 0x2601;

    constexpr GLenum GL_NEAREST = 0x2600;

    constexpr GLenum GL_MODELVIEW = 0x1700;

    constexpr GLenum GL_PROJECTION = 0x1701;

    constexpr GLenum GL_QUADS = 0x0007;

    constexpr GLenum GL_LINES = 0x0001;

    constexpr GLenum GL_TRIANGLES = 0x0004;

    constexpr GLenum GL_FRONT_AND_BACK = 0x0408;

    constexpr GLenum GL_FILL = 0x1B02;

    constexpr GLenum GL_CULL_FACE = 0x0B44;

#define GLPROC(name, ret, args)                                                                    \
    using PFN_##name = ret(*) args;                                                                \
    PFN_##name name = nullptr;
    GLPROC(glViewport, void, (GLint, GLint, GLsizei, GLsizei))
    GLPROC(glMatrixMode, void, (GLenum))
    GLPROC(glLoadIdentity, void, ())
    GLPROC(glOrtho, void, (GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
    GLPROC(glFrustum, void, (GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
    GLPROC(glBegin, void, (GLenum))
    GLPROC(glEnd, void, ())
    GLPROC(glColor4f, void, (GLfloat, GLfloat, GLfloat, GLfloat))
    GLPROC(glVertex2f, void, (GLfloat, GLfloat))
    GLPROC(glVertex3f, void, (GLfloat, GLfloat, GLfloat))
    GLPROC(glTexCoord2f, void, (GLfloat, GLfloat))
    GLPROC(glBindTexture, void, (GLenum, GLuint))
    GLPROC(glGenTextures, void, (GLsizei, GLuint*))
    GLPROC(glDeleteTextures, void, (GLsizei, const GLuint*))
    GLPROC(glTexParameteri, void, (GLenum, GLenum, GLint))
    GLPROC(glTexImage2D,
           void,
           (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
    GLPROC(glEnable, void, (GLenum))
    GLPROC(glDisable, void, (GLenum))
    GLPROC(glClearColor, void, (GLfloat, GLfloat, GLfloat, GLfloat))
    GLPROC(glClear, void, (GLbitfield))
    GLPROC(glTranslatef, void, (GLfloat, GLfloat, GLfloat))
    GLPROC(glRotatef, void, (GLfloat, GLfloat, GLfloat, GLfloat))
    GLPROC(glScalef, void, (GLfloat, GLfloat, GLfloat))
    GLPROC(glPushMatrix, void, ())
    GLPROC(glPopMatrix, void, ())
    GLPROC(glLineWidth, void, (GLfloat))
    GLPROC(glPolygonMode, void, (GLenum, GLenum))
#undef GLPROC

#define SDLPROC(name, ret, args)                                                                   \
    using PFN_SDL_##name = ret(*) args;                                                            \
    PFN_SDL_##name name = nullptr;
    SDLPROC(SDL_Init, int, (uint32_t))
    SDLPROC(SDL_Quit, void, ())
    SDLPROC(SDL_GetError, const char*, ())
    SDLPROC(SDL_CreateWindow, SDL_Window*, (const char*, int, int, int, int, uint32_t))
    SDLPROC(SDL_DestroyWindow, void, (SDL_Window*))
    SDLPROC(SDL_GL_SetAttribute, int, (int, int))
    SDLPROC(SDL_GL_CreateContext, SDL_GLContext, (SDL_Window*))
    SDLPROC(SDL_GL_DeleteContext, void, (SDL_GLContext))
    SDLPROC(SDL_GL_SwapWindow, void, (SDL_Window*))
    SDLPROC(SDL_GL_GetProcAddress, void*, (const char*))
    SDLPROC(SDL_SetWindowTitle, void, (SDL_Window*, const char*))
    SDLPROC(SDL_GetWindowSize, void, (SDL_Window*, int*, int*))
    SDLPROC(SDL_GetWindowFlags, uint32_t, (SDL_Window*))
    SDLPROC(SDL_SetWindowFullscreen, int, (SDL_Window*, uint32_t))
    SDLPROC(SDL_MaximizeWindow, void, (SDL_Window*))
    SDLPROC(SDL_RestoreWindow, void, (SDL_Window*))
    SDLPROC(SDL_CreateRenderer, SDL_Renderer*, (SDL_Window*, int, uint32_t))
    SDLPROC(SDL_DestroyRenderer, void, (SDL_Renderer*))
    SDLPROC(SDL_SetRenderDrawColor, int, (SDL_Renderer*, uint8_t, uint8_t, uint8_t, uint8_t))
    SDLPROC(SDL_RenderClear, int, (SDL_Renderer*))
    SDLPROC(SDL_RenderPresent, void, (SDL_Renderer*))
    SDLPROC(SDL_RenderFillRect, int, (SDL_Renderer*, const SDL_Rect*))
    SDLPROC(SDL_RenderDrawLine, int, (SDL_Renderer*, int, int, int, int))
    SDLPROC(SDL_RenderCopy, int, (SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*))
    SDLPROC(SDL_CreateTexture, SDL_Texture*, (SDL_Renderer*, uint32_t, int, int, int))
    SDLPROC(SDL_CreateTextureFromSurface, SDL_Texture*, (SDL_Renderer*, SDL_Surface*))
    SDLPROC(SDL_DestroyTexture, void, (SDL_Texture*))
    SDLPROC(SDL_UpdateTexture, int, (SDL_Texture*, const SDL_Rect*, const void*, int))
    SDLPROC(SDL_SetHint, int, (const char*, const char*))
    SDLPROC(SDL_SetTextureBlendMode, int, (SDL_Texture*, int))
    SDLPROC(SDL_SetRenderDrawBlendMode, int, (SDL_Renderer*, int))
    SDLPROC(SDL_RenderSetLogicalSize, int, (SDL_Renderer*, int, int))
    SDLPROC(SDL_RenderSetViewport, int, (SDL_Renderer*, const SDL_Rect*))
    SDLPROC(SDL_GL_SetSwapInterval, int, (int))
    SDLPROC(SDL_PollEvent, int, (SDL_Event*))
    SDLPROC(SDL_GetKeyboardState, const uint8_t*, (int*))
    SDLPROC(SDL_GetMouseState, uint32_t, (int*, int*))
    SDLPROC(SDL_ShowCursor, int, (int))
    SDLPROC(SDL_StartTextInput, void, ())
    SDLPROC(SDL_StopTextInput, void, ())
    SDLPROC(SDL_LoadBMP, SDL_Surface*, (const char*))
    SDLPROC(SDL_ConvertSurfaceFormat, SDL_Surface*, (SDL_Surface*, uint32_t, uint32_t))
    SDLPROC(SDL_FreeSurface, void, (SDL_Surface*))
    SDLPROC(SDL_GetPerformanceCounter, uint64_t, ())
    SDLPROC(SDL_GetPerformanceFrequency, uint64_t, ())
    SDLPROC(SDL_OpenAudioDevice,
            SDL_AudioDeviceID,
            (const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int))
    SDLPROC(SDL_CloseAudioDevice, void, (SDL_AudioDeviceID))
    SDLPROC(SDL_PauseAudioDevice, void, (SDL_AudioDeviceID, int))
    SDLPROC(SDL_QueueAudio, int, (SDL_AudioDeviceID, const void*, uint32_t))
    SDLPROC(SDL_GetQueuedAudioSize, uint32_t, (SDL_AudioDeviceID))
    SDLPROC(SDL_ClearQueuedAudio, void, (SDL_AudioDeviceID))
    SDLPROC(SDL_GetNumAudioDevices, int, (int))
#undef SDLPROC

    using PFN_IMG_Load = SDL_Surface* (*)(const char*);

    using PFN_IMG_GetError = const char* (*)();

    static PFN_IMG_Load IMG_Load = nullptr;

    static PFN_IMG_GetError IMG_GetError = nullptr;

    using PFN_TTF_Init = int (*)();

    using PFN_TTF_Quit = void (*)();

    using PFN_TTF_GetError = const char* (*)();

    using PFN_TTF_OpenFont = TTF_Font* (*)(const char*, int);

    using PFN_TTF_CloseFont = void (*)(TTF_Font*);

    using PFN_TTF_RenderUTF8_Blended = SDL_Surface* (*)(TTF_Font*, const char*, SDL_Color);

    using PFN_TTF_SizeUTF8 = int (*)(TTF_Font*, const char*, int*, int*);

    using PFN_TTF_FontHeight = int (*)(TTF_Font*);

    using PFN_TTF_SetFontStyle = void (*)(TTF_Font*, int);

    static PFN_TTF_Init TTF_Init = nullptr;

    static PFN_TTF_Quit TTF_Quit = nullptr;

    static PFN_TTF_GetError TTF_GetError = nullptr;

    static PFN_TTF_OpenFont TTF_OpenFont = nullptr;

    static PFN_TTF_CloseFont TTF_CloseFont = nullptr;

    static PFN_TTF_RenderUTF8_Blended TTF_RenderUTF8_Blended = nullptr;

    static PFN_TTF_SizeUTF8 TTF_SizeUTF8 = nullptr;

    static PFN_TTF_FontHeight TTF_FontHeight = nullptr;

    static PFN_TTF_SetFontStyle TTF_SetFontStyle = nullptr;

    using PFN_Mix_OpenAudio = int (*)(int, uint16_t, int, int);

    using PFN_Mix_CloseAudio = void (*)();

    using PFN_Mix_GetError = const char* (*)();

    using PFN_Mix_LoadMUS = Mix_Music* (*)(const char*);

    using PFN_Mix_FreeMusic = void (*)(Mix_Music*);

    using PFN_Mix_PlayMusic = int (*)(Mix_Music*, int);

    using PFN_Mix_HaltMusic = int (*)();

    using PFN_Mix_VolumeMusic = int (*)(int);

    static PFN_Mix_OpenAudio Mix_OpenAudio = nullptr;

    static PFN_Mix_CloseAudio Mix_CloseAudio = nullptr;

    static PFN_Mix_GetError Mix_GetError = nullptr;

    static PFN_Mix_LoadMUS Mix_LoadMUS = nullptr;

    static PFN_Mix_FreeMusic Mix_FreeMusic = nullptr;

    static PFN_Mix_PlayMusic Mix_PlayMusic = nullptr;

    static PFN_Mix_HaltMusic Mix_HaltMusic = nullptr;

    static PFN_Mix_VolumeMusic Mix_VolumeMusic = nullptr;

    using FnLoadLibrary = void* (*)(const char*);

    using FnGetProc = void* (*)(void*, const char*);

    static void* gSDL = nullptr;

    static void* gIMG = nullptr;

    static void* gTTF = nullptr;

    static void* gMIX = nullptr;

    static SDL_Window* gWindow = nullptr;

    // Cache visibility to avoid redundant SDL cursor-state changes.
    static int gCursorVisibleState = -1;

    static SDL_GLContext gGL = nullptr;

    static bool gGraphics = false;

    static bool gAudio = false;

    static std::string gGraphicsError;

    static bool gClose = false;

    static int gWidth = 1280;

    static int gHeight = 720;

    static int gLastEvent = 0;

    static int gLastKey = -1;

    static int gLastMouseButton = 0;

    static int gLastX = 0;

    static int gLastY = 0;

    static double gLastFrame = 0.0;

    static std::vector<uint8_t> gKeys(512, 0);
    // Keys pressed during the current event-poll frame (edge-triggered).
    static std::vector<uint8_t> gKeyPressed(512, 0);
    // UTF-8 text from SDL_TEXTINPUT events, collected once per poll frame.
    static std::string gTextInputThisFrame;

    static std::array<uint8_t, 8> gMouseButtons{};

    // Buttons pressed during the current frame (edge-triggered click helper).
    static std::array<uint8_t, 8> gMouseClickedButtons{};

    static SDL_AudioDeviceID gAudioDevice = 0;
    static SDL_Renderer* gRenderer = nullptr;
    static bool gSoftware2D = false;
    static bool gOpenGL3D = false;

    static SDL_AudioSpec gAudioSpec{};

    static int gAudioVolume = 128;

    static std::string gAudioError;

    struct Texture {
        GLuint id{};
        SDL_Texture* software{};
        int w{};
        int h{};
    };

    struct CanvasSnapshot {
        std::vector<uint8_t> pixels;
    };

    struct CanvasData {

        int w{};

        int h{};

        GLuint tex{};
        SDL_Texture* software{};

        std::vector<uint8_t> pixels;

        std::vector<CanvasSnapshot> undo;

        std::vector<CanvasSnapshot> redo;
    };

    struct Sound {

        std::vector<uint8_t> data;

        int freq{};

        uint16_t format{};

        uint8_t channels{};

        uint16_t samples{};
    };

    static std::unordered_map<int64_t, Texture> gTextures;

    static std::unordered_map<int64_t, CanvasData> gCanvases;

    static int64_t gNextCanvas = 1;

    static std::unordered_map<int64_t, Sound> gSounds;

    static int64_t gNextTexture = 1;

    static int64_t gNextSound = 1;

    static float gCameraX = 0.0f, gCameraY = 0.0f, gCameraZoom = 1.0f;

    static double gUiWidth = 0.0;

    static double gUiHeight = 0.0;

    static int gViewportX = 0;

    static int gViewportY = 0;

    static int gViewportWidth = 0;

    static int gViewportHeight = 0;

    static std::unordered_map<int64_t, TTF_Font*> gFonts;

    static int64_t gNextFont = 1;

    static Mix_Music* gMusic = nullptr;

    static bool gTTFReady = false;

    static bool gMixerReady = false;

    static bool gUiSliderDragging = false;
    static double gUiSliderX = 0.0;
    static double gUiSliderY = 0.0;
    static double gUiSliderWidth = 0.0;
    static double gUiSliderHeight = 0.0;

    // Immediate-mode text field focus is keyed by its rectangle. Apps should
    // keep each field's rectangle stable while the control remains visible.
    static bool gUiTextFieldFocused = false;
    static double gUiTextFieldX = 0.0;
    static double gUiTextFieldY = 0.0;
    static double gUiTextFieldWidth = 0.0;
    static double gUiTextFieldHeight = 0.0;

    struct EcsEntity {

        bool alive = true;
        float x = 0, y = 0, vx = 0, vy = 0, w = 48, h = 48;

        float r = 0.25f, g = 0.75f, b = 1.0f, a = 1.0f;
        int64_t texture = 0;
    };

    static std::unordered_map<int64_t, EcsEntity> gEntities;

    static int64_t gNextEntity = 1;

    struct SpriteAnim {

        int64_t texture = 0;

        int64_t frameW = 0, frameH = 0, columns = 1, frameCount = 1;

        double fps = 8.0;

        bool loop = true;

        double time = 0.0;

        int64_t frame = 0;
    };

    static std::unordered_map<int64_t, SpriteAnim> gAnims;

    static int64_t gNextAnim = 1;

    struct SceneData {

        std::vector<EcsEntity> entities;
    };

    static std::unordered_map<int64_t, SceneData> gScenes;

    static int64_t gNextScene = 1;

    struct Model3D {

        std::vector<float> vertices;
        // xyz triples, triangle list
    };

    static std::unordered_map<int64_t, Model3D> gModels;

    static int64_t gNextModel = 1;

    struct Particle {

        bool alive = true;
        float x = 0, y = 0, vx = 0, vy = 0, life = 1, maxLife = 1, size = 6;

        float r = 1, g = 1, b = 1, a = 1;
    };

    static std::unordered_map<int64_t, Particle> gParticles;

    static int64_t gNextParticle = 1;

    static void* load_library(const char* name) {

#if defined(_WIN32)
        return reinterpret_cast<void*>(LoadLibraryA(name));

#else
        return dlopen(name, RTLD_NOW | RTLD_LOCAL);

#endif
    }
    static void* load_symbol(void* lib, const char* name) {

#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(lib), name));

#else
        return dlsym(lib, name);

#endif
    }
    static std::string sdl_error() {
        return gSDL && SDL_GetError ? SDL_GetError() : "SDL2 unavailable";
    }
    static void* load_optional(const char* const* names, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            if (void* h = load_library(names[i]))
                return h;
        }
        return nullptr;
    }
    static void load_optional_media_libs() {

#if defined(_WIN32)
        const char* imgNames[] = {"SDL2_image.dll", "SDL2_image-2.0.dll"};

        const char* ttfNames[] = {"SDL2_ttf.dll", "SDL2_ttf-2.0.dll"};

        const char* mixNames[] = {"SDL2_mixer.dll", "SDL2_mixer-2.0.dll"};

#elif defined(__APPLE__)
        const char* imgNames[] = {"libSDL2_image-2.0.0.dylib", "libSDL2_image.dylib"};

        const char* ttfNames[] = {"libSDL2_ttf-2.0.0.dylib", "libSDL2_ttf.dylib"};

        const char* mixNames[] = {"libSDL2_mixer-2.0.0.dylib", "libSDL2_mixer.dylib"};

#else
        const char* imgNames[] = {
            "libSDL2_image-2.0.so.0", "libSDL2_image.so", "libSDL2_image-2.0.so"};

        const char* ttfNames[] = {"libSDL2_ttf-2.0.so.0", "libSDL2_ttf.so", "libSDL2_ttf-2.0.so"};

        const char* mixNames[] = {
            "libSDL2_mixer-2.0.so.0", "libSDL2_mixer.so", "libSDL2_mixer-2.0.so"};

#endif
        if (!gIMG)
            gIMG = load_optional(imgNames, sizeof(imgNames) / sizeof(imgNames[0]));

        if (gIMG && !IMG_Load) {
            IMG_Load = reinterpret_cast<PFN_IMG_Load>(load_symbol(gIMG, "IMG_Load"));
            IMG_GetError = reinterpret_cast<PFN_IMG_GetError>(load_symbol(gIMG, "IMG_GetError"));
        }
        if (!gTTF)
            gTTF = load_optional(ttfNames, sizeof(ttfNames) / sizeof(ttfNames[0]));

        if (gTTF && !TTF_Init) {
            TTF_Init = reinterpret_cast<PFN_TTF_Init>(load_symbol(gTTF, "TTF_Init"));
            TTF_Quit = reinterpret_cast<PFN_TTF_Quit>(load_symbol(gTTF, "TTF_Quit"));
            TTF_GetError = reinterpret_cast<PFN_TTF_GetError>(load_symbol(gTTF, "TTF_GetError"));
            TTF_OpenFont = reinterpret_cast<PFN_TTF_OpenFont>(load_symbol(gTTF, "TTF_OpenFont"));
            TTF_CloseFont = reinterpret_cast<PFN_TTF_CloseFont>(load_symbol(gTTF, "TTF_CloseFont"));
            TTF_RenderUTF8_Blended = reinterpret_cast<PFN_TTF_RenderUTF8_Blended>(
                load_symbol(gTTF, "TTF_RenderUTF8_Blended"));
            TTF_SizeUTF8 =
                reinterpret_cast<PFN_TTF_SizeUTF8>(load_symbol(gTTF, "TTF_SizeUTF8"));
            TTF_FontHeight =
                reinterpret_cast<PFN_TTF_FontHeight>(load_symbol(gTTF, "TTF_FontHeight"));
            TTF_SetFontStyle =
                reinterpret_cast<PFN_TTF_SetFontStyle>(load_symbol(gTTF, "TTF_SetFontStyle"));
        }
        if (!gMIX)
            gMIX = load_optional(mixNames, sizeof(mixNames) / sizeof(mixNames[0]));

        if (gMIX && !Mix_OpenAudio) {
            Mix_OpenAudio = reinterpret_cast<PFN_Mix_OpenAudio>(load_symbol(gMIX, "Mix_OpenAudio"));
            Mix_CloseAudio =
                reinterpret_cast<PFN_Mix_CloseAudio>(load_symbol(gMIX, "Mix_CloseAudio"));
            Mix_GetError = reinterpret_cast<PFN_Mix_GetError>(load_symbol(gMIX, "Mix_GetError"));
            Mix_LoadMUS = reinterpret_cast<PFN_Mix_LoadMUS>(load_symbol(gMIX, "Mix_LoadMUS"));
            Mix_FreeMusic = reinterpret_cast<PFN_Mix_FreeMusic>(load_symbol(gMIX, "Mix_FreeMusic"));
            Mix_PlayMusic = reinterpret_cast<PFN_Mix_PlayMusic>(load_symbol(gMIX, "Mix_PlayMusic"));
            Mix_HaltMusic = reinterpret_cast<PFN_Mix_HaltMusic>(load_symbol(gMIX, "Mix_HaltMusic"));
            Mix_VolumeMusic =
                reinterpret_cast<PFN_Mix_VolumeMusic>(load_symbol(gMIX, "Mix_VolumeMusic"));
        }
    }
    static bool load_sdl() {

        if (gSDL)
            return true;

#if defined(_WIN32)
        const char* names[] = {"SDL2.dll", "SDL2-2.0.dll"};

#elif defined(__APPLE__)
        const char* names[] = {"libSDL2-2.0.0.dylib", "libSDL2.dylib"};

#else
        const char* names[] = {"libSDL2-2.0.so.0", "libSDL2.so", "libSDL2-2.0.so"};

#endif
        for (const char* n : names) {
            if ((gSDL = load_library(n)))
                break;
        }
        if (!gSDL) {
            gGraphicsError = "SDL2 runtime library not found";
            return false;
        }
        bool ok = true;

#define LOAD_REQUIRED_SDL(n)                                                                       \
    do {                                                                                           \
        n = reinterpret_cast<PFN_SDL_##n>(load_symbol(gSDL, #n));                                  \
        if (!n)                                                                                    \
            ok = false;                                                                            \
    } while (0)
#define LOAD_OPTIONAL_SDL(n)                                                                       \
    do {                                                                                           \
        n = reinterpret_cast<PFN_SDL_##n>(load_symbol(gSDL, #n));                                  \
    } while (0)
        // Only video/window/event/timing entry points are required to start graphics.
        // Image and audio entry points are optional and are loaded lazily when those
        // features are actually used. This avoids rejecting valid SDL2 runtimes.
        LOAD_REQUIRED_SDL(SDL_Init);
        LOAD_REQUIRED_SDL(SDL_Quit);
        LOAD_REQUIRED_SDL(SDL_GetError);

        LOAD_REQUIRED_SDL(SDL_CreateWindow);
        LOAD_REQUIRED_SDL(SDL_DestroyWindow);
        LOAD_REQUIRED_SDL(SDL_SetWindowTitle);
        LOAD_REQUIRED_SDL(SDL_GetWindowSize);
        LOAD_OPTIONAL_SDL(SDL_GetWindowFlags);
        LOAD_OPTIONAL_SDL(SDL_SetWindowFullscreen);
        LOAD_OPTIONAL_SDL(SDL_MaximizeWindow);
        LOAD_OPTIONAL_SDL(SDL_RestoreWindow);
        LOAD_REQUIRED_SDL(SDL_CreateRenderer);
        LOAD_REQUIRED_SDL(SDL_DestroyRenderer);
        LOAD_REQUIRED_SDL(SDL_SetRenderDrawColor);
        LOAD_REQUIRED_SDL(SDL_RenderClear);
        LOAD_REQUIRED_SDL(SDL_RenderPresent);
        LOAD_REQUIRED_SDL(SDL_RenderFillRect);
        LOAD_REQUIRED_SDL(SDL_RenderDrawLine);
        LOAD_REQUIRED_SDL(SDL_RenderCopy);
        LOAD_REQUIRED_SDL(SDL_CreateTexture);
        LOAD_REQUIRED_SDL(SDL_CreateTextureFromSurface);
        LOAD_REQUIRED_SDL(SDL_DestroyTexture);
        LOAD_REQUIRED_SDL(SDL_UpdateTexture);
        LOAD_OPTIONAL_SDL(SDL_SetHint);
        LOAD_REQUIRED_SDL(SDL_SetTextureBlendMode);
        LOAD_OPTIONAL_SDL(SDL_SetRenderDrawBlendMode);
        LOAD_REQUIRED_SDL(SDL_RenderSetLogicalSize);
        LOAD_REQUIRED_SDL(SDL_RenderSetViewport);
        LOAD_REQUIRED_SDL(SDL_GL_SetSwapInterval);

        LOAD_REQUIRED_SDL(SDL_GL_SetAttribute);
        LOAD_REQUIRED_SDL(SDL_GL_CreateContext);
        LOAD_REQUIRED_SDL(SDL_GL_DeleteContext);
        LOAD_REQUIRED_SDL(SDL_GL_SwapWindow);
        LOAD_REQUIRED_SDL(SDL_GL_GetProcAddress);

        LOAD_REQUIRED_SDL(SDL_PollEvent);
        LOAD_REQUIRED_SDL(SDL_GetKeyboardState);
        LOAD_REQUIRED_SDL(SDL_GetMouseState);
        LOAD_REQUIRED_SDL(SDL_ShowCursor);
        LOAD_OPTIONAL_SDL(SDL_StartTextInput);
        LOAD_OPTIONAL_SDL(SDL_StopTextInput);

        LOAD_REQUIRED_SDL(SDL_GetPerformanceCounter);
        LOAD_REQUIRED_SDL(SDL_GetPerformanceFrequency);

        LOAD_OPTIONAL_SDL(SDL_LoadBMP);
        LOAD_OPTIONAL_SDL(SDL_ConvertSurfaceFormat);
        LOAD_OPTIONAL_SDL(SDL_FreeSurface);

        LOAD_OPTIONAL_SDL(SDL_OpenAudioDevice);
        LOAD_OPTIONAL_SDL(SDL_CloseAudioDevice);
        LOAD_OPTIONAL_SDL(SDL_PauseAudioDevice);
        LOAD_OPTIONAL_SDL(SDL_QueueAudio);
        LOAD_OPTIONAL_SDL(SDL_GetQueuedAudioSize);
        LOAD_OPTIONAL_SDL(SDL_ClearQueuedAudio);
        LOAD_OPTIONAL_SDL(SDL_GetNumAudioDevices);

#undef LOAD_OPTIONAL_SDL
#undef LOAD_REQUIRED_SDL
        if (!ok) {
            gGraphicsError = "SDL2 runtime is missing required video/window symbols";
            gSDL = nullptr;
            return false;
        }
        load_optional_media_libs();

        return true;
    }

    static bool load_gl() {

        if (!SDL_GL_GetProcAddress)
            return false;

#define LOAD_GL(n)                                                                                 \
    do {                                                                                           \
        n = reinterpret_cast<PFN_##n>(SDL_GL_GetProcAddress(#n));                                  \
        if (!n)                                                                                    \
            return false;                                                                          \
    } while (0)
        LOAD_GL(glViewport);
        LOAD_GL(glMatrixMode);
        LOAD_GL(glLoadIdentity);
        LOAD_GL(glOrtho);
        LOAD_GL(glFrustum);

        LOAD_GL(glBegin);
        LOAD_GL(glEnd);
        LOAD_GL(glColor4f);
        LOAD_GL(glVertex2f);
        LOAD_GL(glVertex3f);
        LOAD_GL(glTexCoord2f);

        LOAD_GL(glBindTexture);
        LOAD_GL(glGenTextures);
        LOAD_GL(glDeleteTextures);
        LOAD_GL(glTexParameteri);
        LOAD_GL(glTexImage2D);

        LOAD_GL(glEnable);
        LOAD_GL(glDisable);
        LOAD_GL(glPushMatrix);
        LOAD_GL(glPopMatrix);
        LOAD_GL(glClearColor);
        LOAD_GL(glClear);
        LOAD_GL(glTranslatef);
        LOAD_GL(glRotatef);
        LOAD_GL(glScalef);
        LOAD_GL(glLineWidth);
        LOAD_GL(glPolygonMode);

#undef LOAD_GL
        return true;
    }

    static double now_seconds() {

        if (SDL_GetPerformanceCounter && SDL_GetPerformanceFrequency) {

            static uint64_t start = SDL_GetPerformanceCounter();

            return double(SDL_GetPerformanceCounter() - start) /
                   double(SDL_GetPerformanceFrequency());
        }
        return double(std::clock()) / double(CLOCKS_PER_SEC);
    }

    static void apply_software_layout() {
        if (!gRenderer)
            return;
        const int logicalW = gUiWidth > 0.0 ? int(std::llround(gUiWidth)) : gWidth;
        const int logicalH = gUiHeight > 0.0 ? int(std::llround(gUiHeight)) : gHeight;
        SDL_RenderSetLogicalSize(gRenderer, logicalW, logicalH);
        SDL_RenderSetViewport(gRenderer, nullptr);
        // SDL's software renderer defaults to SDL_BLENDMODE_NONE, which silently
        // ignores the alpha channel of the draw colour. Enable blending so
        // translucent rectangles, overlays, circles and bitmap text compose
        // correctly instead of painting as opaque blocks.
        if (SDL_SetRenderDrawBlendMode)
            SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
    }

    static void software_color(double r, double g, double b, double a) {
        if (!gRenderer)
            return;
        SDL_SetRenderDrawColor(gRenderer,
                               uint8_t(std::clamp(r, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(g, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(b, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(a, 0.0, 1.0) * 255.0));
    }

    static SDL_Rect software_rect(double x, double y, double w, double h) {
        return SDL_Rect{int(std::llround(x)),
                        int(std::llround(y)),
                        std::max(0, int(std::llround(w))),
                        std::max(0, int(std::llround(h)))};
    }

    static void software_fill_rect(double x, double y, double w, double h) {
        if (!gRenderer)
            return;
        SDL_Rect rect = software_rect(x, y, w, h);
        SDL_RenderFillRect(gRenderer, &rect);
    }

    static void update_window_metrics() {

        if (!gWindow || !SDL_GetWindowSize) {

            return;
        }

        int width = 0;

        int height = 0;

        SDL_GetWindowSize(gWindow, &width, &height);

        if (width > 0) {

            gWidth = width;
        }
        if (height > 0) {

            gHeight = height;
        }

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        gViewportX = 0;

        gViewportY = 0;

        gViewportWidth = gWidth;

        gViewportHeight = gHeight;

        if (gUiWidth <= 0.0 || gUiHeight <= 0.0 || gWidth <= 0 || gHeight <= 0) {

            return;
        }

        const double scaleX = static_cast<double>(gWidth) / logicalWidth;

        const double scaleY = static_cast<double>(gHeight) / logicalHeight;

        const double scale = std::min(scaleX, scaleY);

        gViewportWidth = std::max(1, static_cast<int>(std::llround(logicalWidth * scale)));

        gViewportHeight = std::max(1, static_cast<int>(std::llround(logicalHeight * scale)));

        gViewportX = (gWidth - gViewportWidth) / 2;

        gViewportY = (gHeight - gViewportHeight) / 2;
    }

    static void apply_2d_viewport() {

        update_window_metrics();

        glViewport(gViewportX, gViewportY, gViewportWidth, gViewportHeight);
    }

    static void window_to_ui(double windowX, double windowY, double& uiX, double& uiY) {

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        if (gUiWidth <= 0.0 || gUiHeight <= 0.0 || gViewportWidth <= 0 || gViewportHeight <= 0) {

            uiX = windowX;

            uiY = windowY;

            return;
        }

        uiX = (windowX - static_cast<double>(gViewportX)) * logicalWidth /
              static_cast<double>(gViewportWidth);

        uiY = (windowY - static_cast<double>(gViewportY)) * logicalHeight /
              static_cast<double>(gViewportHeight);
    }

    // glBlendFunc is loaded separately to keep the backend tiny but complete.
    using PFN_glBlendFunc = void (*)(GLenum, GLenum);

    static PFN_glBlendFunc glBlendFuncPtr = nullptr;

    static void setup_2d() {

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);

        if (glBlendFuncPtr) {

            glBlendFuncPtr(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glMatrixMode(GL_PROJECTION);

        glLoadIdentity();

        glOrtho(0.0, logicalWidth, logicalHeight, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();

        if (gCameraZoom != 1.0f || gCameraX != 0.0f || gCameraY != 0.0f) {

            glTranslatef(static_cast<float>(logicalWidth * 0.5),
                         static_cast<float>(logicalHeight * 0.5),
                         0.0f);

            glScalef(gCameraZoom, gCameraZoom, 1.0f);

            glTranslatef(static_cast<float>(-logicalWidth * 0.5 - gCameraX),
                         static_cast<float>(-logicalHeight * 0.5 - gCameraY),
                         0.0f);
        }
    }

    static void draw_quad(float x, float y, float w, float h, float r, float g, float b, float a) {

        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    }

    static void draw_circle(float cx, float cy, float radius, float r, float g, float b, float a) {

        glColor4f(r, g, b, a);
        glBegin(GL_TRIANGLES);
        constexpr int N = 48;
        for (int i = 0; i < N; i++) {
            double a0 = (2.0 * M_PI * i) / N, a1 = (2.0 * M_PI * (i + 1)) / N;
            glVertex2f(cx, cy);
            glVertex2f(float(cx + std::cos(a0) * radius), float(cy + std::sin(a0) * radius));
            glVertex2f(float(cx + std::cos(a1) * radius), float(cy + std::sin(a1) * radius));
        }
        glEnd();
    }

    static const uint8_t FONT5X7[][7] = {

        {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
        {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
        {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F}, // C
        {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, // D
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
        {0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
        {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}, // I
        {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}, // J
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
        {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
        {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
        {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, // W
        {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
        {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // Y
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
        {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
        {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
        {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}, // 3
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
        {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}, // 5
        {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
        {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
    };

    static const uint8_t* glyph(char c) {

        c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;

        if (c >= 'A' && c <= 'Z')
            return FONT5X7[c - 'A'];

        if (c >= '0' && c <= '9')
            return FONT5X7[26 + (c - '0')];

        return nullptr;
    }
    static void
    draw_text(float x, float y, float scale, const char* text, float r, float g, float b, float a) {

        if (!text)
            return;
        float ox = x;
        glColor4f(r, g, b, a);

        for (const char* p = text; *p; p++) {

            if (*p == '\n') {
                y += 8.0f * scale;
                x = ox;
                continue;
            }
            if (*p == ' ') {
                x += 6.0f * scale;
                continue;
            }
            const auto* glyphBits = glyph(*p);
            if (!glyphBits) {
                x += 6.0f * scale;
                continue;
            }
            for (int row = 0; row < 7; row++)
                for (int col = 0; col < 5; col++)
                    if (glyphBits[row] & (1u << (4 - col)))
                        draw_quad(x + col * scale, y + row * scale, scale, scale, r, g, b, a);

            x += 6.0f * scale;
        }
    }

    static void draw_textured_quad(const Texture& t,
                                   float x,
                                   float y,
                                   float w,
                                   float h,
                                   float tintR,
                                   float tintG,
                                   float tintB,
                                   float tintA) {

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, t.id);
        glColor4f(tintR, tintG, tintB, tintA);
        glBegin(GL_QUADS);

        glTexCoord2f(0, 0);
        glVertex2f(x, y);
        glTexCoord2f(1, 0);
        glVertex2f(x + w, y);
        glTexCoord2f(1, 1);
        glVertex2f(x + w, y + h);
        glTexCoord2f(0, 1);
        glVertex2f(x, y + h);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    static int64_t scancode_for(const char* name) {

        if (!name)
            return -1;
        std::string s(name);
        std::transform(
            s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::toupper(c)); });

        if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z') {
            static const int codes[26] = {4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                                          17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};
            return codes[s[0] - 'A'];
        }
        if (s.size() == 1 && s[0] >= '0' && s[0] <= '9') {
            static const int codes[10] = {39, 30, 31, 32, 33, 34, 35, 36, 37, 38};
            return codes[s[0] - '0'];
        }
        if (s == "SPACE")
            return 44;
        if (s == "ENTER")
            return 40;
        if (s == "ESC" || s == "ESCAPE")
            return 41;
        if (s == "LEFT")
            return 80;
        if (s == "RIGHT")
            return 79;
        if (s == "UP")
            return 82;
        if (s == "DOWN")
            return 81;
        if (s == "TAB")
            return 43;
        if (s.size() >= 2 && s[0] == 'F' && s[1] >= '1' && s[1] <= '9') {
            // SDL scancodes for F1..F10 are 58..67; F11/F12 are 68/69.
            int n = 0;
            for (size_t i = 1; i < s.size(); ++i) {
                if (s[i] < '0' || s[i] > '9')
                    return -1;
                n = n * 10 + (s[i] - '0');
            }
            if (n >= 1 && n <= 10)
                return 58 + (n - 1);
            if (n == 11)
                return 68;
            if (n == 12)
                return 69;
            return -1;
        }
        if (s == "SHIFT")
            return 225;
        if (s == "CTRL" || s == "CONTROL")
            return 224;
        return -1;
    }

    static void clear_event() {
        gLastEvent = 0;
        gLastKey = -1;
        gLastMouseButton = 0;
        std::fill(gKeyPressed.begin(), gKeyPressed.end(), uint8_t(0));
        gMouseClickedButtons.fill(0);
        gTextInputThisFrame.clear();
    }
    static void process_events() {

        if (!SDL_PollEvent)
            return;
        SDL_Event e{};

        clear_event();

        while (SDL_PollEvent(&e)) {

            gLastEvent = int(e.type);

            if (e.type == SDL_QUIT) {
                gClose = true;
            } else if (e.type == SDL_TEXTINPUT) {
                size_t textLength = 0;
                while (textLength < sizeof(e.text.text) && e.text.text[textLength] != '\0')
                    ++textLength;
                gTextInputThisFrame.append(e.text.text, textLength);
            } else if (e.type == SDL_KEYDOWN) {
                gLastKey = int(e.key.keysym.scancode);
                if (gLastKey >= 0 && gLastKey < int(gKeys.size())) {
                    gKeys[size_t(gLastKey)] = 1;
                    if (e.key.repeat == 0)
                        gKeyPressed[size_t(gLastKey)] = 1;
                }
                // Escape closes the window by default (SDL_SCANCODE_ESCAPE == 41).
                if (gLastKey == 41)
                    gClose = true;
            } else if (e.type == SDL_KEYUP) {
                gLastKey = int(e.key.keysym.scancode);
                if (gLastKey >= 0 && gLastKey < int(gKeys.size()))
                    gKeys[size_t(gLastKey)] = 0;
            } else if (e.type == SDL_MOUSEMOTION) {
                gLastX = e.motion.x;
                gLastY = e.motion.y;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                gLastMouseButton = int(e.button.button);
                gLastX = e.button.x;
                gLastY = e.button.y;
                if (e.button.button < gMouseButtons.size())
                    gMouseButtons[e.button.button] = 1;
                if (e.button.button < gMouseClickedButtons.size())
                    gMouseClickedButtons[e.button.button] = 1;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == 1)
                    gUiSliderDragging = false;
                gLastMouseButton = int(e.button.button);
                gLastX = e.button.x;
                gLastY = e.button.y;
                if (e.button.button < gMouseButtons.size())
                    gMouseButtons[e.button.button] = 0;
            } else if (e.type == SDL_WINDOWEVENT) {
                gLastX = e.window.data1;
                gLastY = e.window.data2;
                if (e.window.data1 > 0)
                    gWidth = e.window.data1;
                if (e.window.data2 > 0)
                    gHeight = e.window.data2;
            }
        }
    }

    static uint16_t rd16(const uint8_t* p) {
        return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
    }
    static uint32_t rd32(const uint8_t* p) {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
               (uint32_t(p[3]) << 24);
    }
    static bool load_pcm_wav(const char* path,
                             SDL_AudioSpec& spec,
                             std::vector<uint8_t>& audio,
                             std::string& err) {

        std::FILE* f = std::fopen(path, "rb");

        if (!f) {
            err = std::string("cannot open WAV: ") + path;
            return false;
        }
        std::vector<uint8_t> bytes;

        std::fseek(f, 0, SEEK_END);

        long n = std::ftell(f);

        std::rewind(f);

        if (n <= 0) {
            std::fclose(f);
            err = "WAV file is empty";
            return false;
        }
        bytes.resize(size_t(n));

        if (std::fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) {
            std::fclose(f);
            err = "cannot read WAV";
            return false;
        }
        std::fclose(f);

        if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
            std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
            err = "not a RIFF/WAVE file";
            return false;
        }
        uint16_t audioFormat = 0, channels = 0, bits = 0;

        uint32_t sampleRate = 0, dataOffset = 0, dataSize = 0;

        size_t pos = 12;

        while (pos + 8 <= bytes.size()) {

            const uint8_t* h = bytes.data() + pos;

            uint32_t chunkSize = rd32(h + 4);

            size_t payload = pos + 8;

            if (payload > bytes.size())
                break;

            size_t available = bytes.size() - payload;

            size_t actual = std::min<size_t>(chunkSize, available);

            if (std::memcmp(h, "fmt ", 4) == 0 && actual >= 16) {

                const uint8_t* q = bytes.data() + payload;

                audioFormat = rd16(q + 0);

                channels = rd16(q + 2);

                sampleRate = rd32(q + 4);

                bits = rd16(q + 14);

            } else if (std::memcmp(h, "data", 4) == 0) {

                dataOffset = uint32_t(payload);

                dataSize = uint32_t(actual);
            }
            pos = payload + actual + (actual & 1u);
        }
        if (audioFormat != 1) {
            err = "only PCM WAV is supported without SDL2_mixer";
            return false;
        }
        if (channels == 0 || channels > 2 || sampleRate == 0) {
            err = "unsupported WAV channel/sample-rate configuration";
            return false;
        }
        if (bits != 8 && bits != 16) {
            err = "only 8-bit or 16-bit PCM WAV is supported";
            return false;
        }
        if (dataSize == 0 || dataOffset + dataSize > bytes.size()) {
            err = "WAV data chunk is missing or invalid";
            return false;
        }
        spec = SDL_AudioSpec{};

        spec.freq = int(sampleRate);

        spec.channels = uint8_t(channels);

        spec.samples = 4096;

        spec.padding = 0;

        spec.callback = nullptr;

        spec.userdata = nullptr;

        if (bits == 8)
            spec.format = 0x0008u;
        // AUDIO_U8
        else
            spec.format = 0x8010u;
        // AUDIO_S16LSB
        spec.size = dataSize;

        audio.assign(bytes.begin() + dataOffset, bytes.begin() + dataOffset + dataSize);

        return true;
    }

    static bool ensure_audio_device(const SDL_AudioSpec& wanted) {

        if (gAudioDevice)
            return true;

        if (!SDL_OpenAudioDevice || !SDL_PauseAudioDevice) {
            gAudioError = "SDL2 audio queue API unavailable";
            return false;
        }
        SDL_AudioSpec obtained{};

        constexpr int ALLOW_FREQUENCY_CHANGE = 0x0001;

        constexpr int ALLOW_FORMAT_CHANGE = 0x0002;

        constexpr int ALLOW_CHANNELS_CHANGE = 0x0004;

        const int allowed = ALLOW_FREQUENCY_CHANGE | ALLOW_FORMAT_CHANGE | ALLOW_CHANNELS_CHANGE;

        gAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, allowed);

        if (!gAudioDevice) {
            gAudioError = std::string("SDL_OpenAudioDevice failed: ") + sdl_error();
            return false;
        }
        gAudioSpec = obtained;

        SDL_PauseAudioDevice(gAudioDevice, 0);

        return true;
    }

    static float pcm_sample(const uint8_t* p, uint16_t format) {

        if (format == 0x0008u)
            return (float(int(*p) - 128)) / 128.0f;

        if (format == 0x8010u) {
            int16_t v = int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
            return float(v) / 32768.0f;
        }

        if (format == 0x8120u) {
            float v = 0.0f;
            std::memcpy(&v, p, sizeof(v));
            return std::clamp(v, -1.0f, 1.0f);
        }
        return 0.0f;
    }

    static std::vector<uint8_t> convert_pcm(const std::vector<uint8_t>& src,
                                            const SDL_AudioSpec& in,
                                            const SDL_AudioSpec& out) {

        const int inBytes = (in.format == 0x0008u) ? 1
                            : (in.format == 0x8010u) ? 2
                            : (in.format == 0x8120u) ? 4
                                                     : 0;

        const int outBytes = (out.format == 0x0008u) ? 1
                             : (out.format == 0x8010u) ? 2
                             : (out.format == 0x8120u) ? 4
                                                       : 0;

        if (inBytes == 0 || outBytes == 0 || in.channels == 0 || out.channels == 0 ||
            in.freq <= 0 || out.freq <= 0)
            return {};

        const size_t inFrameBytes = size_t(inBytes) * in.channels;

        if (inFrameBytes == 0 || src.size() < inFrameBytes)
            return {};

        const size_t inFrames = src.size() / inFrameBytes;

        const size_t outFrames = std::max<size_t>(
            1, size_t(std::llround(double(inFrames) * double(out.freq) / double(in.freq))));

        std::vector<float> inF(inFrames * in.channels, 0.0f);

        for (size_t f = 0; f < inFrames; ++f) {

            for (unsigned c = 0; c < in.channels; ++c)
                inF[f * in.channels + c] =
                    pcm_sample(src.data() + f * inFrameBytes + c * inBytes, in.format);
        }
        std::vector<float> outF(outFrames * out.channels, 0.0f);

        for (size_t of = 0; of < outFrames; ++of) {

            double srcPos = double(of) * double(in.freq) / double(out.freq);

            size_t i0 = size_t(std::min<double>(std::floor(srcPos), double(inFrames - 1)));

            size_t i1 = std::min(i0 + 1, inFrames - 1);

            float frac = float(srcPos - double(i0));

            auto chan = [&](unsigned c) -> float {
                if (in.channels == 1)
                    return inF[i0] * (1.0f - frac) + inF[i1] * frac;

                if (out.channels == 1) {

                    float a0 = inF[i0 * in.channels + 0],
                          a1 = inF[i0 * in.channels + std::min<unsigned>(1, in.channels - 1)];

                    float b0 = inF[i1 * in.channels + 0],
                          b1 = inF[i1 * in.channels + std::min<unsigned>(1, in.channels - 1)];

                    return ((a0 + a1) * 0.5f) * (1.0f - frac) + ((b0 + b1) * 0.5f) * frac;
                }
                unsigned srcC = std::min<unsigned>(c, in.channels - 1);

                return inF[i0 * in.channels + srcC] * (1.0f - frac) +
                       inF[i1 * in.channels + srcC] * frac;
            };

            for (unsigned c = 0; c < out.channels; ++c)
                outF[of * out.channels + c] = std::clamp(chan(c), -1.0f, 1.0f);
        }
        std::vector<uint8_t> converted(outF.size() * size_t(outBytes));

        for (size_t i = 0; i < outF.size(); ++i) {

            if (out.format == 0x8120u) {

                float v = outF[i];
                std::memcpy(converted.data() + i * 4, &v, sizeof(v));

            } else if (out.format == 0x8010u) {

                int v = int(std::lrint(std::clamp(outF[i], -1.0f, 1.0f) * 32767.0f));

                converted[i * 2] = uint8_t(v & 0xff);
                converted[i * 2 + 1] = uint8_t((uint16_t(v) >> 8) & 0xff);

            } else {

                int v = 128 + int(std::lrint(std::clamp(outF[i], -1.0f, 1.0f) * 127.0f));

                converted[i] = uint8_t(std::clamp(v, 0, 255));
            }
        }
        return converted;
    }

    static bool init_graphics(int w, int h, const char* title) {
        if (gGraphics)
            return true;
        gGraphicsError.clear();
        gCursorVisibleState = -1;
        gSoftware2D = false;
        gOpenGL3D = false;
        if (!load_sdl())
            return false;
        // Keep window size and pointer coordinates in the same physical-pixel
        // space on high-DPI displays. Without this, SDL reports a 1600x900
        // window but DPI-scaled mouse events, so hit-testing drifts by the
        // display scale factor.
        if (SDL_SetHint) {
            SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "permonitorv2");
            SDL_SetHint("SDL_WINDOWS_DPI_SCALING", "0");
        }
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            gGraphicsError = std::string("SDL_Init failed: ") + sdl_error();
            return false;
        }
        gWidth = std::max(w, 320);
        gHeight = std::max(h, 240);
        gWindow = SDL_CreateWindow(title ? title : "NEXUS",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   gWidth,
                                   gHeight,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!gWindow) {
            gGraphicsError = std::string("SDL_CreateWindow failed: ") + sdl_error();
            SDL_Quit();
            return false;
        }
        // The 2D backend intentionally starts with SDL's software renderer.
        // This avoids driver-dependent OpenGL/fixed-pipeline issues and makes
        // Paint/GUI behavior deterministic across desktop and virtual displays.
        gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_SOFTWARE);
        if (!gRenderer) {
            // Fall back to accelerated 2D when software rendering is unavailable.
            gRenderer = SDL_CreateRenderer(
                gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        }
        if (!gRenderer) {
            gGraphicsError = std::string("SDL renderer creation failed: ") + sdl_error();
            SDL_DestroyWindow(gWindow);
            gWindow = nullptr;
            SDL_Quit();
            return false;
        }
        gSoftware2D = true;
        gGraphics = true;
        gClose = false;
        gLastFrame = now_seconds();
        apply_software_layout();
        software_color(0.04, 0.045, 0.065, 1.0);
        SDL_RenderClear(gRenderer);
        SDL_RenderPresent(gRenderer);
        return true;
    }

    static bool ensure_opengl_backend() {
        if (gOpenGL3D)
            return true;
        if (!gWindow || !gRenderer) {
            gGraphicsError = "graphics window is not initialized";
            return false;
        }
        int w = gWidth;
        int h = gHeight;
        if (SDL_GetWindowSize)
            SDL_GetWindowSize(gWindow, &w, &h);
        SDL_DestroyRenderer(gRenderer);
        gRenderer = nullptr;
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        gWindow = SDL_CreateWindow("NEXUS",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   std::max(w, 320),
                                   std::max(h, 240),
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!gWindow) {
            gGraphicsError = std::string("OpenGL window creation failed: ") + sdl_error();
            return false;
        }
        gGL = SDL_GL_CreateContext(gWindow);
        if (!gGL) {
            gGraphicsError = std::string("SDL_GL_CreateContext failed: ") + sdl_error();
            SDL_DestroyWindow(gWindow);
            gWindow = nullptr;
            return false;
        }
        if (!load_gl()) {
            gGraphicsError = "OpenGL 2.1 compatibility functions are unavailable";
            SDL_GL_DeleteContext(gGL);
            gGL = nullptr;
            SDL_DestroyWindow(gWindow);
            gWindow = nullptr;
            return false;
        }
        glBlendFuncPtr = reinterpret_cast<PFN_glBlendFunc>(SDL_GL_GetProcAddress("glBlendFunc"));
        gWidth = std::max(w, 320);
        gHeight = std::max(h, 240);
        gSoftware2D = false;
        gOpenGL3D = true;
        apply_2d_viewport();
        return true;
    }

    static void shutdown_graphics() {
        if (gUiTextFieldFocused && SDL_StopTextInput)
            SDL_StopTextInput();
        gUiTextFieldFocused = false;
        gTextInputThisFrame.clear();
        if (gMusic && Mix_FreeMusic)
            Mix_FreeMusic(gMusic);
        gMusic = nullptr;
        if (Mix_CloseAudio && gMixerReady)
            Mix_CloseAudio();
        gMixerReady = false;
        if (gAudioDevice && SDL_CloseAudioDevice)
            SDL_CloseAudioDevice(gAudioDevice);
        gAudioDevice = 0;
        gAudio = false;
        for (auto& [id, font] : gFonts) {
            if (TTF_CloseFont && font)
                TTF_CloseFont(font);
        }
        gFonts.clear();
        if (TTF_Quit && gTTFReady)
            TTF_Quit();
        gTTFReady = false;
        for (auto& [id, texture] : gTextures) {
            if (texture.id && glDeleteTextures && gOpenGL3D)
                glDeleteTextures(1, &texture.id);
            if (texture.software && SDL_DestroyTexture)
                SDL_DestroyTexture(texture.software);
        }
        gTextures.clear();
        for (auto& [id, canvas] : gCanvases) {
            if (canvas.tex && glDeleteTextures && gOpenGL3D)
                glDeleteTextures(1, &canvas.tex);
            if (canvas.software && SDL_DestroyTexture)
                SDL_DestroyTexture(canvas.software);
        }
        gCanvases.clear();
        gSounds.clear();
        gModels.clear();
        gParticles.clear();
        gAnims.clear();
        gScenes.clear();
        gEntities.clear();
        if (gGL && SDL_GL_DeleteContext)
            SDL_GL_DeleteContext(gGL);
        gGL = nullptr;
        if (gRenderer && SDL_DestroyRenderer)
            SDL_DestroyRenderer(gRenderer);
        gRenderer = nullptr;
        if (gWindow && SDL_DestroyWindow)
            SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
        if (SDL_Quit)
            SDL_Quit();
        gGraphics = false;
        gSoftware2D = false;
        gOpenGL3D = false;
    }

} // namespace

namespace {

    static std::string nx_shell_quote(const char* s) {

        std::string out = "'";

        if (!s)
            return out + "'";

        for (const char c : std::string(s)) {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += "'";
        return out;
    }

    struct SQLiteApi {

        using Db = void;

        using Open = int (*)(const char*, Db**);

        using Exec = int (*)(Db*, const char*, int (*)(void*, int, char**, char**), void*, char**);

        using Close = int (*)(Db*);

        void* handle = nullptr;
        Open open = nullptr;
        Exec exec = nullptr;
        Close close = nullptr;
    };

    static SQLiteApi gSqlite;

    static std::unordered_map<int64_t, SQLiteApi::Db*> gDatabases;

    static int64_t gNextDb = 1;

    static bool load_sqlite_runtime() {

        if (gSqlite.open)
            return true;

#if defined(_WIN32)
        HMODULE h = LoadLibraryA("sqlite3.dll");

        if (!h)
            return false;

        gSqlite.handle = h;

        gSqlite.open = reinterpret_cast<SQLiteApi::Open>(GetProcAddress(h, "sqlite3_open"));

        gSqlite.exec = reinterpret_cast<SQLiteApi::Exec>(GetProcAddress(h, "sqlite3_exec"));

        gSqlite.close = reinterpret_cast<SQLiteApi::Close>(GetProcAddress(h, "sqlite3_close"));

#else
        void* h = dlopen("libsqlite3.so.0", RTLD_LAZY);

        if (!h)
            h = dlopen("libsqlite3.so", RTLD_LAZY);

        if (!h)
            return false;

        gSqlite.handle = h;

        gSqlite.open = reinterpret_cast<SQLiteApi::Open>(dlsym(h, "sqlite3_open"));

        gSqlite.exec = reinterpret_cast<SQLiteApi::Exec>(dlsym(h, "sqlite3_exec"));

        gSqlite.close = reinterpret_cast<SQLiteApi::Close>(dlsym(h, "sqlite3_close"));

#endif
        return gSqlite.open && gSqlite.exec && gSqlite.close;
    }
} // namespace

namespace {
    struct NxManagedBlock {
        size_t size = 0;
        bool scanContents = false;
        std::thread::id owner;
    };
    struct NxManagedBlockView {
        uintptr_t base = 0;
        void* pointer = nullptr;
        size_t size = 0;
        bool scanContents = false;
        bool marked = false;
    };
    static std::unordered_map<void*, NxManagedBlock> gNxManagedBlocks;
    static std::mutex gNxManagedMutex;
    static constexpr size_t kNxGcThreshold = 512u * 1024u;
    static constexpr size_t kNxGcMaxScannedBlock = 1024u * 1024u;
    static thread_local size_t gNxBytesSinceCollection = 0;
    static thread_local bool gNxCollecting = false;
    static uintptr_t nx_stack_high() {
#if defined(_WIN32)
        auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
        return tib ? reinterpret_cast<uintptr_t>(tib->StackBase) : 0;
#elif defined(__APPLE__)
        return reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
#elif defined(__linux__)
        pthread_attr_t attr;
        if (pthread_getattr_np(pthread_self(), &attr) != 0)
            return 0;
        void* base = nullptr;
        size_t size = 0;
        const int status = pthread_attr_getstack(&attr, &base, &size);
        pthread_attr_destroy(&attr);
        return status == 0 ? reinterpret_cast<uintptr_t>(base) + size : 0;
#else
        return 0;
#endif
    }
    static void nx_gc_collect();
    static void* nx_managed_alloc(size_t size, bool scanContents = false) {
        if (size == 0)
            size = 1;
        if (!gNxCollecting && gNxBytesSinceCollection >= kNxGcThreshold)
            nx_gc_collect();
        void* pointer = std::malloc(size);
        if (!pointer)
            return nullptr;
        {
            std::lock_guard<std::mutex> lock(gNxManagedMutex);
            gNxManagedBlocks.emplace(
                pointer, NxManagedBlock{size, scanContents, std::this_thread::get_id()});
        }
        if (size > std::numeric_limits<size_t>::max() - gNxBytesSinceCollection)
            gNxBytesSinceCollection = kNxGcThreshold;
        else
            gNxBytesSinceCollection += size;
        return pointer;
    }
    static void* nx_managed_realloc(void* pointer, size_t size) {
        if (!pointer)
            return nx_managed_alloc(size);
        if (size == 0)
            size = 1;
        if (!gNxCollecting && gNxBytesSinceCollection >= kNxGcThreshold)
            nx_gc_collect();
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        auto old = gNxManagedBlocks.find(pointer);
        void* resized = std::realloc(pointer, size);
        if (!resized)
            return nullptr;
        if (old != gNxManagedBlocks.end()) {
            NxManagedBlock block = old->second;
            gNxManagedBlocks.erase(old);
            block.size = size;
            gNxManagedBlocks[resized] = block;
        }
        if (size > std::numeric_limits<size_t>::max() - gNxBytesSinceCollection)
            gNxBytesSinceCollection = kNxGcThreshold;
        else
            gNxBytesSinceCollection += size;
        return resized;
    }
    static void nx_managed_free(void* pointer) {
        if (!pointer)
            return;
        {
            std::lock_guard<std::mutex> lock(gNxManagedMutex);
            gNxManagedBlocks.erase(pointer);
        }
        std::free(pointer);
    }
    // Conservative collection scans arbitrary machine words from the active stack.
    // ASan deliberately poisons stack red-zones, although those bytes are part of the
    // stack range this collector walks. Keep this tiny byte-loader uninstrumented;
    // the actual mark/sweep logic and heap-object scanning remain sanitizer-instrumented.
#if defined(__clang__)
    __attribute__((no_sanitize("address")))
#elif defined(__GNUC__)
    __attribute__((no_sanitize_address))
#elif defined(_MSC_VER)
    __declspec(no_sanitize_address)
#endif
    static uintptr_t
    nx_gc_read_stack_word(uintptr_t address) {
        const volatile unsigned char* input =
            reinterpret_cast<const volatile unsigned char*>(address);
        uintptr_t value = 0;
        unsigned char* output = reinterpret_cast<unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); ++i)
            output[i] = input[i];
        return value;
    }

    static void nx_gc_collect() {
        if (gNxCollecting)
            return;
        gNxCollecting = true;
        struct CollectGuard {
            ~CollectGuard() {
                gNxCollecting = false;
            }
        } guard;
        volatile uintptr_t stackMarker = 0;
        const uintptr_t stackStart = reinterpret_cast<uintptr_t>(&stackMarker);
        const uintptr_t stackEnd = nx_stack_high();
        if (stackEnd <= stackStart) {
            gNxBytesSinceCollection = 0;
            return;
        }
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        const auto owner = std::this_thread::get_id();
        std::vector<NxManagedBlockView> blocks;
        blocks.reserve(gNxManagedBlocks.size());
        for (const auto& [pointer, block] : gNxManagedBlocks) {
            if (block.owner != owner)
                continue;
            blocks.push_back({reinterpret_cast<uintptr_t>(pointer),
                              pointer,
                              block.size,
                              block.scanContents,
                              false});
        }
        std::sort(blocks.begin(), blocks.end(), [](const auto& left, const auto& right) {
            return left.base < right.base;
        });
        std::vector<size_t> work;
        work.reserve(blocks.size() / 4u + 1u);
        auto markCandidate = [&](uintptr_t candidate) {
            auto it = std::upper_bound(blocks.begin(),
                                       blocks.end(),
                                       candidate,
                                       [](uintptr_t value, const NxManagedBlockView& block) {
                                           return value < block.base;
                                       });
            if (it == blocks.begin())
                return;
            --it;
            if (candidate < it->base || candidate - it->base >= it->size || it->marked)
                return;
            it->marked = true;
            const size_t index = static_cast<size_t>(it - blocks.begin());
            if (it->scanContents && it->size <= kNxGcMaxScannedBlock)
                work.push_back(index);
        };
        auto scanRange = [&](uintptr_t begin, uintptr_t end) {
            constexpr uintptr_t wordSize = sizeof(uintptr_t);
            begin = (begin + wordSize - 1u) & ~(wordSize - 1u);
            for (uintptr_t at = begin; at <= end && end - at >= wordSize; at += wordSize) {
                uintptr_t candidate = 0;
                std::memcpy(&candidate, reinterpret_cast<const void*>(at), sizeof(candidate));
                markCandidate(candidate);
            }
        };
        auto scanStackRange = [&](uintptr_t begin, uintptr_t end) {
            constexpr uintptr_t wordSize = sizeof(uintptr_t);
            begin = (begin + wordSize - 1u) & ~(wordSize - 1u);
            for (uintptr_t at = begin; at <= end && end - at >= wordSize; at += wordSize) {
                markCandidate(nx_gc_read_stack_word(at));
            }
        };
        std::jmp_buf registerSnapshot;
        (void)setjmp(registerSnapshot);
        scanStackRange(stackStart, stackEnd);
        scanRange(reinterpret_cast<uintptr_t>(&registerSnapshot),
                  reinterpret_cast<uintptr_t>(&registerSnapshot) + sizeof(registerSnapshot));
        for (size_t next = 0; next < work.size(); ++next) {
            const auto& block = blocks[work[next]];
            scanRange(block.base, block.base + block.size);
        }
        for (const auto& block : blocks) {
            if (!block.marked) {
                auto it = gNxManagedBlocks.find(block.pointer);
                if (it != gNxManagedBlocks.end() && it->second.owner == owner) {
                    std::free(block.pointer);
                    gNxManagedBlocks.erase(it);
                }
            }
        }
        gNxBytesSinceCollection = 0;
    }
#ifdef NEXUS_RUNTIME_TESTING
    static size_t nx_test_live_managed_bytes() {
        std::lock_guard<std::mutex> lock(gNxManagedMutex);
        size_t total = 0;
        const auto owner = std::this_thread::get_id();
        for (const auto& [pointer, block] : gNxManagedBlocks) {
            (void)pointer;
            if (block.owner == owner)
                total += block.size;
        }
        return total;
    }
#endif

    // A bounded, strict JSON parser shared by the runtime JSON helpers. Source
    // spans let object/array values be returned as JSON without re-serializing.
    struct NxJsonValue {
        enum class Kind { Null, Boolean, Number, String, Object, Array };
        Kind kind = Kind::Null;
        size_t start = 0;
        size_t end = 0;
        std::string stringValue;
        std::vector<std::string> keys;
        std::vector<NxJsonValue> children;
    };

    class NxJsonParser {
    public:
        explicit NxJsonParser(std::string_view input) : input_(input) {}

        bool parse(NxJsonValue& root) {
            if (input_.size() > kMaxInputBytes)
                return false;
            skipWhitespace();
            if (!parseValue(root, 0))
                return false;
            skipWhitespace();
            return position_ == input_.size();
        }

    private:
        static constexpr size_t kMaxInputBytes = 16u * 1024u * 1024u;
        static constexpr size_t kMaxDepth = 128;
        static constexpr size_t kMaxNodes = 100'000;

        std::string_view input_;
        size_t position_ = 0;
        size_t nodes_ = 0;

        void skipWhitespace() {
            while (position_ < input_.size()) {
                const char c = input_[position_];
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
                    break;
                ++position_;
            }
        }

        static void appendCodePoint(std::string& out, uint32_t codePoint) {
            if (codePoint <= 0x7f) {
                out.push_back(static_cast<char>(codePoint));
            } else if (codePoint <= 0x7ff) {
                out.push_back(static_cast<char>(0xc0u | (codePoint >> 6)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            } else if (codePoint <= 0xffff) {
                out.push_back(static_cast<char>(0xe0u | (codePoint >> 12)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            } else {
                out.push_back(static_cast<char>(0xf0u | (codePoint >> 18)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
            }
        }

        bool readHex4(uint32_t& value) {
            if (input_.size() - position_ < 4)
                return false;
            value = 0;
            for (size_t i = 0; i < 4; ++i) {
                const unsigned char c = static_cast<unsigned char>(input_[position_++]);
                value <<= 4;
                if (c >= '0' && c <= '9')
                    value |= c - '0';
                else if (c >= 'a' && c <= 'f')
                    value |= c - 'a' + 10u;
                else if (c >= 'A' && c <= 'F')
                    value |= c - 'A' + 10u;
                else
                    return false;
            }
            return true;
        }

        bool appendRawUtf8(std::string& out) {
            const size_t start = position_;
            const unsigned char first = static_cast<unsigned char>(input_[position_]);
            size_t width = 0;
            uint32_t codePoint = 0;
            uint32_t minimum = 0;
            if (first >= 0xc2 && first <= 0xdf) {
                width = 2;
                codePoint = first & 0x1fu;
                minimum = 0x80;
            } else if (first >= 0xe0 && first <= 0xef) {
                width = 3;
                codePoint = first & 0x0fu;
                minimum = 0x800;
            } else if (first >= 0xf0 && first <= 0xf4) {
                width = 4;
                codePoint = first & 0x07u;
                minimum = 0x10000;
            } else {
                return false;
            }
            if (input_.size() - position_ < width)
                return false;
            for (size_t i = 1; i < width; ++i) {
                const unsigned char next = static_cast<unsigned char>(input_[position_ + i]);
                if ((next & 0xc0u) != 0x80u)
                    return false;
                codePoint = (codePoint << 6) | (next & 0x3fu);
            }
            if (codePoint < minimum || codePoint > 0x10ffff ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff))
                return false;
            out.append(input_.data() + start, width);
            position_ += width;
            return true;
        }

        bool parseString(std::string& out) {
            if (position_ >= input_.size() || input_[position_] != '"')
                return false;
            ++position_;
            out.clear();
            while (position_ < input_.size()) {
                const unsigned char c = static_cast<unsigned char>(input_[position_++]);
                if (c == '"')
                    return true;
                if (c < 0x20)
                    return false;
                if (c >= 0x80) {
                    --position_;
                    if (!appendRawUtf8(out))
                        return false;
                    continue;
                }
                if (c != '\\') {
                    out.push_back(static_cast<char>(c));
                    continue;
                }
                if (position_ >= input_.size())
                    return false;
                const char escaped = input_[position_++];
                switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    uint32_t codePoint = 0;
                    if (!readHex4(codePoint))
                        return false;
                    if (codePoint >= 0xd800 && codePoint <= 0xdbff) {
                        if (input_.size() - position_ < 6 || input_[position_] != '\\' ||
                            input_[position_ + 1] != 'u')
                            return false;
                        position_ += 2;
                        uint32_t low = 0;
                        if (!readHex4(low) || low < 0xdc00 || low > 0xdfff)
                            return false;
                        codePoint = 0x10000u + ((codePoint - 0xd800u) << 10) + (low - 0xdc00u);
                    } else if (codePoint >= 0xdc00 && codePoint <= 0xdfff) {
                        return false;
                    }
                    appendCodePoint(out, codePoint);
                    break;
                }
                default:
                    return false;
                }
            }
            return false;
        }

        bool parseNumber() {
            if (position_ < input_.size() && input_[position_] == '-')
                ++position_;
            if (position_ >= input_.size())
                return false;
            if (input_[position_] == '0') {
                ++position_;
                if (position_ < input_.size() && input_[position_] >= '0' &&
                    input_[position_] <= '9')
                    return false;
            } else {
                if (input_[position_] < '1' || input_[position_] > '9')
                    return false;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
            }
            if (position_ < input_.size() && input_[position_] == '.') {
                ++position_;
                const size_t fractionStart = position_;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
                if (position_ == fractionStart)
                    return false;
            }
            if (position_ < input_.size() &&
                (input_[position_] == 'e' || input_[position_] == 'E')) {
                ++position_;
                if (position_ < input_.size() &&
                    (input_[position_] == '+' || input_[position_] == '-'))
                    ++position_;
                const size_t exponentStart = position_;
                while (position_ < input_.size() && input_[position_] >= '0' &&
                       input_[position_] <= '9')
                    ++position_;
                if (position_ == exponentStart)
                    return false;
            }
            return true;
        }

        bool parseLiteral(std::string_view literal) {
            if (input_.substr(position_, literal.size()) != literal)
                return false;
            position_ += literal.size();
            return true;
        }

        bool parseValue(NxJsonValue& value, size_t depth) {
            skipWhitespace();
            if (depth > kMaxDepth || ++nodes_ > kMaxNodes || position_ >= input_.size())
                return false;
            value.start = position_;
            const char c = input_[position_];
            if (c == '"') {
                value.kind = NxJsonValue::Kind::String;
                if (!parseString(value.stringValue))
                    return false;
            } else if (c == '{') {
                value.kind = NxJsonValue::Kind::Object;
                ++position_;
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == '}') {
                    ++position_;
                } else {
                    while (true) {
                        skipWhitespace();
                        std::string key;
                        if (!parseString(key))
                            return false;
                        skipWhitespace();
                        if (position_ >= input_.size() || input_[position_++] != ':')
                            return false;
                        NxJsonValue child;
                        if (!parseValue(child, depth + 1))
                            return false;
                        value.keys.push_back(std::move(key));
                        value.children.push_back(std::move(child));
                        skipWhitespace();
                        if (position_ >= input_.size())
                            return false;
                        if (input_[position_] == '}') {
                            ++position_;
                            break;
                        }
                        if (input_[position_++] != ',')
                            return false;
                    }
                }
            } else if (c == '[') {
                value.kind = NxJsonValue::Kind::Array;
                ++position_;
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == ']') {
                    ++position_;
                } else {
                    while (true) {
                        NxJsonValue child;
                        if (!parseValue(child, depth + 1))
                            return false;
                        value.children.push_back(std::move(child));
                        skipWhitespace();
                        if (position_ >= input_.size())
                            return false;
                        if (input_[position_] == ']') {
                            ++position_;
                            break;
                        }
                        if (input_[position_++] != ',')
                            return false;
                    }
                }
            } else if (c == 't') {
                value.kind = NxJsonValue::Kind::Boolean;
                if (!parseLiteral("true"))
                    return false;
            } else if (c == 'f') {
                value.kind = NxJsonValue::Kind::Boolean;
                if (!parseLiteral("false"))
                    return false;
            } else if (c == 'n') {
                value.kind = NxJsonValue::Kind::Null;
                if (!parseLiteral("null"))
                    return false;
            } else {
                value.kind = NxJsonValue::Kind::Number;
                if (!parseNumber())
                    return false;
            }
            value.end = position_;
            return true;
        }
    };

    static bool nx_parse_json_source(const char* input,
                                    std::string& source,
                                    NxJsonValue& root) {
        if (!input)
            return false;
        const size_t length = std::strlen(input);
        if (length > 16u * 1024u * 1024u)
            return false;
        source.assign(input, length);
        return NxJsonParser(source).parse(root);
    }

    static const NxJsonValue* nx_json_find_member(const NxJsonValue& object, const char* key) {
        if (object.kind != NxJsonValue::Kind::Object || !key)
            return nullptr;
        // For duplicate keys, prefer the last occurrence, matching common parsers.
        for (size_t i = object.keys.size(); i > 0; --i)
            if (object.keys[i - 1] == key)
                return &object.children[i - 1];
        return nullptr;
    }

    static std::string nx_json_value_text(const std::string& source, const NxJsonValue& value) {
        if (value.kind == NxJsonValue::Kind::String)
            return value.stringValue;
        return source.substr(value.start, value.end - value.start);
    }

    static const char* nx_json_kind_name(NxJsonValue::Kind kind) {
        switch (kind) {
        case NxJsonValue::Kind::Null: return "null";
        case NxJsonValue::Kind::Boolean: return "boolean";
        case NxJsonValue::Kind::Number: return "number";
        case NxJsonValue::Kind::String: return "string";
        case NxJsonValue::Kind::Object: return "object";
        case NxJsonValue::Kind::Array: return "array";
        }
        return "invalid";
    }

} // namespace

extern "C" {

// Existing runtime API -------------------------------------------------------
static char* nx_strdup(const char* src) {
    if (!src)
        src = "";
    size_t n = std::strlen(src);
    char* out = (char*)nx_managed_alloc(n + 1);
    if (!out)
        return nullptr;
    std::memcpy(out, src, n + 1);
    return out;
}
double nexus_sqrt(double x) {
    return std::sqrt(std::max(0.0, x));
}
double nexus_sin(double x) {
    return std::sin(x);
}
double nexus_cos(double x) {
    return std::cos(x);
}
double nexus_tan(double x) {
    return std::tan(x);
}
double nexus_atan2(double y, double x) {
    return std::atan2(y, x);
}
double nexus_floor(double x) {
    return std::floor(x);
}
double nexus_ceil(double x) {
    return std::ceil(x);
}
double nexus_pow(double base, double exponent) {
    return std::pow(base, exponent);
}
double nexus_to_f64(int64_t x) {
    return double(x);
}
int64_t nexus_to_i64(double x) {
    // Define behavior for values outside llround's representable domain.
    // NaN becomes zero; infinities and out-of-range values saturate.
    if (std::isnan(x))
        return 0;
    const double signedLimit = std::ldexp(1.0, 63); // 2^63, exactly representable.
    if (x >= signedLimit)
        return std::numeric_limits<int64_t>::max();
    if (x <= -signedLimit)
        return std::numeric_limits<int64_t>::min();
    // llround preserves the language's existing nearest-integer, halfway-away-from-zero behavior.
    return static_cast<int64_t>(std::llround(x));
}
void nexus_print_i64(int64_t x) {
    std::printf("%lld\n", (long long)x);
}
void nexus_print_f64(double x) {
    std::printf("%.15g\n", x);
}
void nexus_print_bool(bool x) {
    std::puts(x ? "true" : "false");
}
void nexus_print_str(const char* x) {
    std::puts(x ? x : "null");
}

#ifdef NEXUS_RUNTIME_TESTING
size_t nexus_runtime_test_live_bytes() {
    return nx_test_live_managed_bytes();
}
void nexus_runtime_test_collect() {
    nx_gc_collect();
}
#endif

void* nexus_alloc(size_t n) {
    return nx_managed_alloc(n, n <= kNxGcMaxScannedBlock);
}
void nexus_free(void* p) {
    nx_managed_free(p);
}
void nexus_bounds_check(int64_t i, int64_t n) {
    if (i < 0 || i >= n) {
        std::fprintf(stderr,
                     "Nexus runtime error: array index %lld out of bounds for size %lld\n",
                     (long long)i,
                     (long long)n);
        std::exit(1);
    }
}
char* nexus_str_concat(const char* a, const char* b) {
    if (!a)
        a = "";
    if (!b)
        b = "";
    size_t la = std::strlen(a), lb = std::strlen(b);
    char* out = (char*)nx_managed_alloc(la + lb + 1);
    if (!out)
        return nullptr;
    std::memcpy(out, a, la);
    std::memcpy(out + la, b, lb);
    out[la + lb] = 0;
    return out;
}
bool nexus_str_equal(const char* a, const char* b) {
    return a == b || (!a && !b) || (a && b && std::strcmp(a, b) == 0);
}
static char* nx_readline() {
    size_t cap = 256, len = 0;
    char* buf = (char*)nx_managed_alloc(cap);
    if (!buf)
        return nullptr;
    int ch;
    while ((ch = std::getchar()) != EOF) {
        if (ch == '\r')
            continue;
        if (ch == '\n')
            break;
        if (len + 1 >= cap) {
            cap *= 2;
            char* nb = (char*)nx_managed_realloc(buf, cap);
            if (!nb) {
                nx_managed_free(buf);
                return nullptr;
            }
            buf = nb;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = 0;
    return buf;
}
char* nexus_input(const char* p) {
    if (p)
        std::fputs(p, stdout);
    std::fflush(stdout);
    return nx_readline();
}
int64_t nexus_input_i64(const char* p) {
    for (;;) {
        char* s = nexus_input(p);
        if (!s)
            return 0;
        char* e = nullptr;
        long long v = std::strtoll(s, &e, 10);
        while (e && *e == ' ')
            ++e;
        if (e && *e == 0) {
            nx_managed_free(s);
            return v;
        }
        std::fprintf(stderr, "Invalid integer.\n");
        nx_managed_free(s);
    }
}
double nexus_input_f64(const char* p) {
    for (;;) {
        char* s = nexus_input(p);
        if (!s)
            return 0;
        char* e = nullptr;
        double v = std::strtod(s, &e);
        while (e && *e == ' ')
            ++e;
        if (e && *e == 0) {
            nx_managed_free(s);
            return v;
        }
        std::fprintf(stderr, "Invalid number.\n");
        nx_managed_free(s);
    }
}
char* nexus_str_i64(int64_t v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%lld", (long long)v);
    return nx_strdup(b);
}
char* nexus_str_f64(double v) {
    char b[96];
    std::snprintf(b, sizeof(b), "%.15g", v);
    return nx_strdup(b);
}
char* nexus_str_bool(bool v) {
    return nx_strdup(v ? "true" : "false");
}
void nexus_clear() {
    std::fputs("\033[2J\033[H", stdout);
    std::fflush(stdout);
}
void nexus_sleep_ms(int64_t ms) {
    if (ms <= 0)
        return;

#if defined(_WIN32)
    Sleep((DWORD)ms);

#else
    timespec ts{};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)((ms % 1000) * 1000000);
    nanosleep(&ts, nullptr);

#endif
}
static uint64_t nexus_runtime_random_u64() {
    // SplitMix64: a compact non-cryptographic generator. Atomic state makes
    // concurrent calls well-defined; do not use random_i64 for secrets.
    constexpr uint64_t increment = UINT64_C(0x9E3779B97F4A7C15);
    static std::atomic<uint64_t> state{
        static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (static_cast<uint64_t>(std::time(nullptr)) + increment)};
    uint64_t value = state.fetch_add(increment, std::memory_order_relaxed) + increment;
    value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
    return value ^ (value >> 31);
}

static int64_t nexus_i64_add_unsigned_offset(int64_t base, uint64_t offset) {
    if (base >= 0)
        return base + static_cast<int64_t>(offset);

    // Distance to zero is calculated without negating INT64_MIN.
    const uint64_t untilZero = static_cast<uint64_t>(-(base + 1)) + 1u;
    if (offset < untilZero)
        return base + static_cast<int64_t>(offset);
    return static_cast<int64_t>(offset - untilZero);
}

int64_t nexus_random_i64(int64_t min, int64_t max) {
    if (min > max)
        std::swap(min, max);

    // Unsigned arithmetic avoids signed overflow, including the complete i64
    // domain where the inclusive span is 2^64 and wraps to zero.
    const uint64_t span = static_cast<uint64_t>(max) -
                          static_cast<uint64_t>(min) + uint64_t{1};
    uint64_t offset = 0;
    if (span == 0) {
        offset = nexus_runtime_random_u64();
    } else {
        // Rejection sampling removes modulo bias for bounded intervals.
        const uint64_t threshold = (uint64_t{0} - span) % span;
        uint64_t randomValue = 0;
        do {
            randomValue = nexus_runtime_random_u64();
        } while (randomValue < threshold);
        offset = randomValue % span;
    }
    return nexus_i64_add_unsigned_offset(min, offset);
}
int64_t nexus_time_ms() {
    return int64_t(now_seconds() * 1000.0);
}
int64_t nexus_system(const char* c) {
    return c ? std::system(c) : -1;
}
char* nexus_file_read(const char* p) {
    if (!p)
        return nullptr;
    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return nullptr;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::rewind(f);
    if (n < 0) {
        std::fclose(f);
        return nullptr;
    }
    char* o = (char*)nx_managed_alloc(size_t(n) + 1);
    if (!o) {
        std::fclose(f);
        return nullptr;
    }
    size_t got = std::fread(o, 1, size_t(n), f);
    std::fclose(f);
    o[got] = 0;
    return o;
}
// Checked file read: success is independent of the returned text length, so an
// existing empty file is distinct from an open/read/allocation failure.
bool nexus_file_read_checked(const char* p, char** out) {
    if (!out)
        return false;
    *out = nullptr;
    if (!p)
        return false;

    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return false;
    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return false;
    }
    const long length = std::ftell(f);
    if (length < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    const auto size = static_cast<unsigned long long>(length);
    if (size >= static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) {
        std::fclose(f);
        return false;
    }

    char* data = static_cast<char*>(nx_managed_alloc(static_cast<size_t>(length) + 1));
    if (!data) {
        std::fclose(f);
        return false;
    }
    const size_t expected = static_cast<size_t>(length);
    const size_t read = std::fread(data, 1, expected, f);
    bool ok = read == expected && std::ferror(f) == 0;
    if (std::fclose(f) != 0)
        ok = false;
    if (!ok) {
        nx_managed_free(data);
        return false;
    }
    data[read] = '\0';
    *out = data;
    return true;
}

int64_t nexus_file_write(const char* p, const char* d) {
    if (!p)
        return -1;
    std::FILE* f = std::fopen(p, "wb");
    if (!f)
        return -1;
    const char* s = d ? d : "";
    size_t n = std::strlen(s), w = std::fwrite(s, 1, n, f);
    int ok = w == n && std::fclose(f) == 0;
    return ok ? 0 : -1;
}
bool nexus_file_exists(const char* p) {
    if (!p)
        return false;
    std::FILE* f = std::fopen(p, "rb");
    if (!f)
        return false;
    std::fclose(f);
    return true;
}
char* nexus_env(const char* n) {
    const char* v = n ? std::getenv(n) : nullptr;
    return nx_strdup(v ? v : "");
}
void nexus_exit(int64_t c) {
    std::exit(int(c));
}
int64_t nexus_mem_alloc(int64_t bytes) {
    if (bytes <= 0)
        return 0;
    return reinterpret_cast<int64_t>(nx_managed_alloc(
        static_cast<size_t>(bytes), static_cast<size_t>(bytes) <= kNxGcMaxScannedBlock));
}
void nexus_mem_free(int64_t ptr) {
    if (ptr)
        nx_managed_free(reinterpret_cast<void*>(ptr));
}

void nexus_mem_set(int64_t ptr, int64_t value, int64_t bytes) {
    if (ptr == 0 || bytes <= 0)
        return;
    std::memset(reinterpret_cast<void*>(ptr), int(value) & 0xff, static_cast<size_t>(bytes));
}

int64_t nexus_mem_read(int64_t address) {
    if (!address)
        return 0;
    return *reinterpret_cast<int64_t*>(address);
}
void nexus_mem_copy(int64_t destination, int64_t source, int64_t bytes) {
    if (destination == 0 || source == 0 || bytes <= 0)
        return;
    std::memcpy(reinterpret_cast<void*>(destination),
                reinterpret_cast<const void*>(source),
                static_cast<size_t>(bytes));
}
char* nexus_http_get(const char* url) {
    if (!url || !*url)
        return nx_strdup("");

    // Keep the response bounded: this API returns a single managed string and
    // has no streaming interface. Pass the URL as an argv item, never through a
    // command shell (notably, single-quote escaping is not safe for cmd.exe).
    constexpr size_t maxResponseBytes = 16u * 1024u * 1024u;
#if defined(_WIN32)
    const int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, nullptr, 0);
    if (wideLength <= 1)
        return nx_strdup("");
    std::wstring wideUrl(static_cast<size_t>(wideLength), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1,
                            wideUrl.data(), wideLength) == 0)
        return nx_strdup("");
    wideUrl.pop_back();

    const auto quoteArgument = [](const std::wstring& argument) {
        std::wstring quoted(1, L'"');
        size_t backslashes = 0;
        for (const wchar_t c : argument) {
            if (c == L'\\') {
                ++backslashes;
            } else if (c == L'"') {
                quoted.append(backslashes * 2u + 1u, L'\\');
                quoted.push_back(L'"');
                backslashes = 0;
            } else {
                quoted.append(backslashes, L'\\');
                quoted.push_back(c);
                backslashes = 0;
            }
        }
        quoted.append(backslashes * 2u, L'\\');
        quoted.push_back(L'"');
        return quoted;
    };

    const std::wstring command =
        quoteArgument(L"curl.exe") + L" -fsSL --max-time 15 -- " + quoteArgument(wideUrl);
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &attributes, 0))
        return nx_strdup("");
    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        return nx_strdup("");
    }
    HANDLE nullInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullInput == INVALID_HANDLE_VALUE) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        return nx_strdup("");
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nullInput;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(nullInput);
    CloseHandle(writePipe);
    if (!created) {
        CloseHandle(readPipe);
        return nx_strdup("");
    }

    std::string output;
    char buffer[4096];
    bool tooLarge = false;
    bool readFailed = false;
    for (;;) {
        DWORD received = 0;
        if (!ReadFile(readPipe, buffer, static_cast<DWORD>(sizeof(buffer)), &received, nullptr)) {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                readFailed = true;
            break;
        }
        if (received == 0)
            break;
        if (output.size() > maxResponseBytes - static_cast<size_t>(received)) {
            tooLarge = true;
            break;
        }
        output.append(buffer, static_cast<size_t>(received));
    }
    CloseHandle(readPipe);
    if (tooLarge || readFailed)
        TerminateProcess(process.hProcess, 1);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (tooLarge || readFailed || exitCode != 0)
        return nx_strdup("");
    return nx_strdup(output.c_str());
#else
    int pipes[2] = {-1, -1};
    if (pipe(pipes) != 0)
        return nx_strdup("");

    const pid_t child = fork();
    if (child < 0) {
        close(pipes[0]);
        close(pipes[1]);
        return nx_strdup("");
    }
    if (child == 0) {
        close(pipes[0]);
        if (dup2(pipes[1], STDOUT_FILENO) < 0)
            _exit(126);
        close(pipes[1]);
        execlp("curl", "curl", "-fsSL", "--max-time", "15", "--", url,
               static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipes[1]);
    std::string output;
    char buffer[4096];
    bool tooLarge = false;
    bool readFailed = false;
    for (;;) {
        const ssize_t received = read(pipes[0], buffer, sizeof(buffer));
        if (received == 0)
            break;
        if (received < 0) {
            if (errno == EINTR)
                continue;
            readFailed = true;
            break;
        }
        if (output.size() > maxResponseBytes - static_cast<size_t>(received)) {
            tooLarge = true;
            break;
        }
        output.append(buffer, static_cast<size_t>(received));
    }
    close(pipes[0]);
    if (tooLarge || readFailed)
        kill(child, SIGTERM);
    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            status = -1;
            break;
        }
    }
    if (tooLarge || readFailed || status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return nx_strdup("");
    return nx_strdup(output.c_str());
#endif
}
bool nexus_json_valid(const char* json) {
    std::string source;
    NxJsonValue root;
    return nx_parse_json_source(json, source, root);
}
char* nexus_json_get(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!key || !nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("");
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value)
        return nx_strdup("");
    const std::string result = nx_json_value_text(source, *value);
    return nx_strdup(result.c_str());
}
char* nexus_json_type(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root))
        return nx_strdup("invalid");
    if (root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("missing");
    const NxJsonValue* value = nx_json_find_member(root, key);
    return nx_strdup(value ? nx_json_kind_name(value->kind) : "missing");
}
int64_t nexus_json_array_len(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return -1;
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value || value->kind != NxJsonValue::Kind::Array)
        return -1;
    return static_cast<int64_t>(value->children.size());
}
char* nexus_json_array_get(const char* json, const char* key, int64_t index) {
    if (index < 0)
        return nx_strdup("");
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("");
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value || value->kind != NxJsonValue::Kind::Array ||
        static_cast<uint64_t>(index) >= value->children.size())
        return nx_strdup("");
    const std::string result =
        nx_json_value_text(source, value->children[static_cast<size_t>(index)]);
    return nx_strdup(result.c_str());
}
int64_t nexus_sqlite_open(const char* path) {

    if (!load_sqlite_runtime())
        return 0;
    SQLiteApi::Db* db = nullptr;
    if (gSqlite.open(path ? path : ":memory:", &db) != 0 || !db)
        return 0;
    const int64_t id = gNextDb++;
    gDatabases[id] = db;
    return id;
}
int64_t nexus_sqlite_exec(int64_t id, const char* sql) {

    auto it = gDatabases.find(id);
    if (it == gDatabases.end() || !gSqlite.exec)
        return -1;
    char* err = nullptr;
    const int rc = gSqlite.exec(it->second, sql ? sql : "", nullptr, nullptr, &err);
    if (err) {
        std::fprintf(stderr, "SQLite: %s\n", err);
        if (std::strlen(err))
            std::free(err);
    }
    return rc;
}
void nexus_sqlite_close(int64_t id) {
    auto it = gDatabases.find(id);
    if (it == gDatabases.end())
        return;
    if (gSqlite.close)
        gSqlite.close(it->second);
    gDatabases.erase(it);
}
// Portable in-process SHA-256 implementation (FIPS 180-4).
// No external commands, temporary files, or platform-specific crypto APIs.
static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
               (uint32_t(block[i * 4 + 2]) << 8) | uint32_t(block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = (W[i - 15] >> 7 | W[i - 15] << 25) ^ (W[i - 15] >> 18 | W[i - 15] << 14) ^
                      (W[i - 15] >> 3);
        uint32_t s1 = (W[i - 2] >> 17 | W[i - 2] << 15) ^ (W[i - 2] >> 19 | W[i - 2] << 13) ^
                      (W[i - 2] >> 10);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = (e >> 6 | e << 26) ^ (e >> 11 | e << 21) ^ (e >> 25 | e << 7);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + K[i] + W[i];
        uint32_t S0 = (a >> 2 | a << 30) ^ (a >> 13 | a << 19) ^ (a >> 22 | a << 10);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

char* nexus_sha256(const char* text) {
    if (!text)
        return nx_strdup("");

    const uint8_t* data = reinterpret_cast<const uint8_t*>(text);
    size_t len = strlen(text);

    uint32_t state[8] = {0x6a09e667,
                         0xbb67ae85,
                         0x3c6ef372,
                         0xa54ff53a,
                         0x510e527f,
                         0x9b05688c,
                         0x1f83d9ab,
                         0x5be0cd19};

    // Process full 64-byte blocks
    size_t i = 0;
    for (; i + 64 <= len; i += 64) {
        sha256_transform(state, data + i);
    }

    // Final block with padding
    uint8_t block[64] = {0};
    size_t remaining = len - i;
    memcpy(block, data + i, remaining);
    block[remaining] = 0x80;

    if (remaining < 56) {
        // Length fits in this block
        uint64_t bit_len = static_cast<uint64_t>(len) * 8;
        for (int j = 0; j < 8; ++j) {
            block[63 - j] = static_cast<uint8_t>(bit_len >> (8 * j));
        }
        sha256_transform(state, block);
    } else {
        // Need two blocks
        sha256_transform(state, block);
        memset(block, 0, 64);
        uint64_t bit_len = static_cast<uint64_t>(len) * 8;
        for (int j = 0; j < 8; ++j) {
            block[63 - j] = static_cast<uint8_t>(bit_len >> (8 * j));
        }
        sha256_transform(state, block);
    }

    // Format as hex string
    char hex[65];
    for (int j = 0; j < 8; ++j) {
        std::snprintf(hex + j * 8, 9, "%08x", state[j]);
    }
    hex[64] = '\0';
    return nx_strdup(hex);
}

} // extern "C"

// os.args_count / os.args_get: command-line arguments (excluding executable name).
// Platform-specific implementation to avoid requiring argc/argv from the compiler.
// Kept outside the C-linkage block because it returns a C++ standard container.
static std::vector<std::string> get_os_args() {
    std::vector<std::string> args;
#if defined(_WIN32)
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wargv && argc > 0) {
        for (int i = 1; i < argc; ++i) { // Skip executable name (index 0)
            int size = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
            if (size > 0) {
                std::string arg(static_cast<size_t>(size), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, &arg[0], size, nullptr, nullptr);
                if (!arg.empty() && arg.back() == '\0')
                    arg.pop_back();
                args.push_back(std::move(arg));
            }
        }
        LocalFree(wargv);
    }
#elif defined(__APPLE__)
    // macOS does not expose Linux's /proc/self/cmdline. Use the system
    // argument vector and skip argv[0], matching the Windows/Linux API.
    int argc = *_NSGetArgc();
    char** argv = *_NSGetArgv();
    for (int i = 1; argv && i < argc; ++i) {
        args.emplace_back(argv[i] ? argv[i] : "");
    }
#else
    // Linux: /proc/self/cmdline is a NUL-separated argument vector.
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    if (cmdline) {
        std::string arg;
        char ch;
        bool first = true;
        while (cmdline.get(ch)) {
            if (ch == '\0') {
                if (!first)
                    args.push_back(arg);
                first = false;
                arg.clear();
            } else {
                arg += ch;
            }
        }
        if (!first && !arg.empty())
            args.push_back(arg);
    }
#endif
    return args;
}

extern "C" {

int64_t nexus_os_args_count() {
    auto args = get_os_args();
    return static_cast<int64_t>(args.size());
}

char* nexus_os_args_get(int64_t index) {
    auto args = get_os_args();
    if (index < 0 || static_cast<size_t>(index) >= args.size()) {
        return nx_strdup("");
    }
    return nx_strdup(args[static_cast<size_t>(index)].c_str());
}

// string.len: returns byte length of string
int64_t nexus_string_len(const char* s) {
    if (!s)
        return 0;
    return static_cast<int64_t>(std::strlen(s));
}

// string.byte_at: returns byte at index (0-255), or -1 if out of bounds
int64_t nexus_string_byte_at(const char* s, int64_t index) {
    if (!s)
        return -1;
    size_t len = std::strlen(s);
    if (index < 0 || static_cast<size_t>(index) >= len)
        return -1;
    return static_cast<int64_t>(static_cast<unsigned char>(s[static_cast<size_t>(index)]));
}

// Decode one UTF-8 code point starting at `index`. Invalid bytes are consumed
// one at a time and reported as their byte value, so callers always make
// progress even on malformed input. Returns false when the sequence is invalid.
static bool nx_utf8_next(const unsigned char* s, size_t len, size_t& index, int64_t& cp) {
    if (index >= len)
        return false;
    const unsigned char first = s[index];
    if (first < 0x80) {
        cp = first;
        ++index;
        return true;
    }

    size_t extra = 0;
    int64_t value = 0;
    if ((first & 0xE0) == 0xC0) {
        extra = 1;
        value = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        extra = 2;
        value = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        extra = 3;
        value = first & 0x07;
    } else {
        cp = first;
        ++index;
        return false;
    }

    if (index + extra >= len) {
        cp = first;
        ++index;
        return false;
    }

    for (size_t k = 1; k <= extra; ++k) {
        const unsigned char next = s[index + k];
        if ((next & 0xC0) != 0x80) {
            cp = first;
            ++index;
            return false;
        }
        value = (value << 6) | (next & 0x3F);
    }

    const bool overlong = (extra == 1 && value < 0x80) || (extra == 2 && value < 0x800) ||
                          (extra == 3 && value < 0x10000);
    const bool surrogate = value >= 0xD800 && value <= 0xDFFF;
    if (overlong || surrogate || value > 0x10FFFF) {
        cp = first;
        ++index;
        return false;
    }

    cp = value;
    index += extra + 1;
    return true;
}

// string.chars: number of UTF-8 code points (invalid bytes count as one each)
int64_t nexus_string_char_count(const char* s) {
    if (!s)
        return 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t index = 0;
    int64_t count = 0;
    while (index < len) {
        int64_t cp = 0;
        (void)nx_utf8_next(bytes, len, index, cp);
        ++count;
    }
    return count;
}

// string.char_at: code point at a character index, or -1 if out of range
int64_t nexus_string_char_at(const char* s, int64_t index) {
    if (!s || index < 0)
        return -1;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t byteIndex = 0;
    int64_t charIndex = 0;
    while (byteIndex < len) {
        int64_t cp = 0;
        (void)nx_utf8_next(bytes, len, byteIndex, cp);
        if (charIndex == index)
            return cp;
        ++charIndex;
    }
    return -1;
}

// string.valid_utf8: true when every byte sequence is a valid UTF-8 code point
bool nexus_string_valid_utf8(const char* s) {
    if (!s)
        return true;
    const auto* bytes = reinterpret_cast<const unsigned char*>(s);
    const size_t len = std::strlen(s);
    size_t index = 0;
    while (index < len) {
        int64_t cp = 0;
        if (!nx_utf8_next(bytes, len, index, cp))
            return false;
    }
    return true;
}

// string.from_char: encode a Unicode code point (invalid values use U+FFFD)
char* nexus_string_from_char(int64_t code) {
    if (code < 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        code = 0xFFFD;

    char out[4];
    size_t n = 0;
    if (code < 0x80) {
        out[n++] = static_cast<char>(code);
    } else if (code < 0x800) {
        out[n++] = static_cast<char>(0xC0 | (code >> 6));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        out[n++] = static_cast<char>(0xE0 | (code >> 12));
        out[n++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    } else {
        out[n++] = static_cast<char>(0xF0 | (code >> 18));
        out[n++] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out[n++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out[n++] = static_cast<char>(0x80 | (code & 0x3F));
    }

    char* result = (char*)nx_managed_alloc(n + 1);
    if (!result)
        return nx_strdup("");
    std::memcpy(result, out, n);
    result[n] = 0;
    return result;
}

// capture_output: run a shell command and return its stdout as a string.
char* nexus_capture_output(const char* cmd) {
    if (!cmd || !*cmd)
        return nx_strdup("");
#if defined(_WIN32)
    std::FILE* p = _popen(cmd, "rb");
#else
    std::FILE* p = popen(cmd, "r");
#endif
    if (!p)
        return nx_strdup("");
    std::string out;
    char buf[4096];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), p)) > 0)
        out.append(buf, n);
#if defined(_WIN32)
    _pclose(p);
#else
    pclose(p);
#endif
    char* result = (char*)nx_managed_alloc(out.size() + 1);
    if (!result)
        return nx_strdup("");
    std::memcpy(result, out.data(), out.size());
    result[out.size()] = 0;
    return result;
}

// string.contains: substring test
bool nexus_string_contains(const char* s, const char* sub) {
    if (!s || !sub)
        return false;
    if (!*sub)
        return true;
    return std::strstr(s, sub) != nullptr;
}

// string.find: byte index of first occurrence, or -1
int64_t nexus_string_find(const char* s, const char* sub) {
    if (!s || !sub)
        return -1;
    const char* hit = std::strstr(s, sub);
    return hit ? static_cast<int64_t>(hit - s) : -1;
}

static bool nx_is_space_byte(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

// string.trim: copy without leading/trailing ASCII whitespace
char* nexus_string_trim(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    size_t b = 0, e = len;
    while (b < e && nx_is_space_byte((unsigned char)s[b]))
        ++b;
    while (e > b && nx_is_space_byte((unsigned char)s[e - 1]))
        --e;
    char* o = (char*)nx_managed_alloc(e - b + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, s + b, e - b);
    o[e - b] = 0;
    return o;
}

// string.upper / string.lower: ASCII case conversion
char* nexus_string_upper(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    char* o = (char*)nx_managed_alloc(len + 1);
    if (!o)
        return nx_strdup("");
    for (size_t i = 0; i < len; ++i)
        o[i] = char(std::toupper((unsigned char)s[i]));
    o[len] = 0;
    return o;
}

char* nexus_string_lower(const char* s) {
    if (!s)
        return nx_strdup("");
    size_t len = std::strlen(s);
    char* o = (char*)nx_managed_alloc(len + 1);
    if (!o)
        return nx_strdup("");
    for (size_t i = 0; i < len; ++i)
        o[i] = char(std::tolower((unsigned char)s[i]));
    o[len] = 0;
    return o;
}

// string.slice: substring by byte range, boundaries clamped to length
char* nexus_string_slice(const char* s, int64_t start, int64_t end) {
    if (!s)
        return nx_strdup("");
    int64_t len = static_cast<int64_t>(std::strlen(s));
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;
    if (start >= end)
        return nx_strdup("");
    char* o = (char*)nx_managed_alloc(size_t(end - start) + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, s + start, size_t(end - start));
    o[end - start] = 0;
    return o;
}

// string.split_count: number of separator-separated fields (>=1)
int64_t nexus_string_split_count(const char* s, const char* sep) {
    if (!s)
        return 1;
    if (!sep || !*sep)
        return 1;
    int64_t count = 1;
    const char* p = s;
    size_t sl = std::strlen(sep);
    while ((p = std::strstr(p, sep)) != nullptr) {
        ++count;
        p += sl;
    }
    return count;
}

// string.split_at: field `index` (0-based) after splitting by separator
char* nexus_string_split_at(const char* s, const char* sep, int64_t index) {
    if (!s)
        return nx_strdup("");
    if (!sep || !*sep || index < 0)
        return nx_strdup("");
    size_t sl = std::strlen(sep);
    const char* p = s;
    for (int64_t i = 0; i < index; ++i) {
        const char* hit = std::strstr(p, sep);
        if (!hit)
            return nx_strdup("");
        p = hit + sl;
    }
    const char* hit = std::strstr(p, sep);
    size_t fieldLen = hit ? size_t(hit - p) : std::strlen(p);
    char* o = (char*)nx_managed_alloc(fieldLen + 1);
    if (!o)
        return nx_strdup("");
    std::memcpy(o, p, fieldLen);
    o[fieldLen] = 0;
    return o;
}

// Growable lists behind opaque i64 handles. Values live until the process
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

// Interactive terminal helpers remain available.
void nexus_screen_begin() {}
void nexus_screen_end() {}
void nexus_screen_clear() {
    nexus_clear();
}
void nexus_screen_put(int64_t x, int64_t y, const char* t) {
    std::printf("\033[%lld;%lldH%s", (long long)(y + 1), (long long)(x + 1), t ? t : "");
}
void nexus_screen_present() {
    std::fflush(stdout);
}
void nexus_screen_set_title(const char* t) {
    (void)t;
}
int64_t nexus_screen_width() {
    return 80;
}
int64_t nexus_screen_height() {
    return 24;
}
bool nexus_key_pressed() {
    return false;
}
int64_t nexus_read_key() {
    return -1;
}
void nexus_beep() {
    std::fputs("\a", stdout);
    std::fflush(stdout);
}

// Window / events ------------------------------------------------------------
bool nexus_gfx_init(int64_t w, int64_t h, const char* title) {
    return init_graphics(int(w), int(h), title);
}
const char* nexus_gfx_error() {
    return gGraphicsError.c_str();
}
void nexus_gfx_shutdown() {
    shutdown_graphics();
}
bool nexus_gfx_should_close() {
    return gClose;
}
void nexus_gfx_poll() {
    process_events();
}
int64_t nexus_gfx_event_type() {
    return gLastEvent;
}
int64_t nexus_gfx_event_key() {
    return gLastKey;
}
int64_t nexus_gfx_event_mouse_button() {
    return gLastMouseButton;
}
int64_t nexus_gfx_event_x() {
    return gLastX;
}
int64_t nexus_gfx_event_y() {
    return gLastY;
}
double nexus_gfx_mouse_x() {

    if (gGraphics && SDL_GetMouseState) {

        int x = 0;

        int y = 0;

        SDL_GetMouseState(&x, &y);

        gLastX = x;

        gLastY = y;
    }

    double uiX = 0.0;

    double uiY = 0.0;

    window_to_ui(static_cast<double>(gLastX), static_cast<double>(gLastY), uiX, uiY);

    return uiX;
}

double nexus_gfx_mouse_y() {

    if (gGraphics && SDL_GetMouseState) {

        int x = 0;

        int y = 0;

        SDL_GetMouseState(&x, &y);

        gLastX = x;

        gLastY = y;
    }

    double uiX = 0.0;

    double uiY = 0.0;

    window_to_ui(static_cast<double>(gLastX), static_cast<double>(gLastY), uiX, uiY);

    return uiY;
}
bool nexus_gfx_mouse_down(int64_t button) {
    if (button < 0 || button >= int64_t(gMouseButtons.size()))
        return false;
    if (gGraphics && SDL_GetMouseState) {
        int x, y;
        uint32_t b = SDL_GetMouseState(&x, &y);
        return button > 0 && (b & (1u << uint32_t(button - 1))) != 0;
    }
    return gMouseButtons[size_t(button)] != 0;
}
// True only on the frame a button went down. Use this for buttons and cards
// so one physical click cannot trigger the same action twice.
bool nexus_gfx_mouse_clicked(int64_t button) {
    if (button < 0 || button >= int64_t(gMouseClickedButtons.size()))
        return false;
    return gMouseClickedButtons[size_t(button)] != 0;
}
bool nexus_gfx_key_down(const char* name) {
    if (!gGraphics || !SDL_GetKeyboardState)
        return false;
    int64_t sc = scancode_for(name);
    if (sc < 0)
        return false;
    int n = 0;
    const uint8_t* k = SDL_GetKeyboardState(&n);
    return sc < n && k != nullptr && k[sc] != 0;
}

bool nexus_gfx_key_pressed(const char* name) {
    if (!gGraphics)
        return false;
    const int64_t sc = scancode_for(name);
    if (sc < 0 || sc >= int64_t(gKeyPressed.size()))
        return false;
    return gKeyPressed[size_t(sc)] != 0;
}
double nexus_gfx_dt() {
    double n = now_seconds();
    double d = gLastFrame ? n - gLastFrame : 0.0;
    gLastFrame = n;
    return d;
}
bool nexus_gfx_event_is(const char* name) {
    if (!name)
        return false;
    std::string s(name);
    if (s == "quit")
        return gLastEvent == int(SDL_QUIT);
    if (s == "key_down")
        return gLastEvent == int(SDL_KEYDOWN);
    if (s == "key_up")
        return gLastEvent == int(SDL_KEYUP);
    if (s == "mouse_move")
        return gLastEvent == int(SDL_MOUSEMOTION);
    if (s == "mouse_down")
        return gLastEvent == int(SDL_MOUSEBUTTONDOWN);
    if (s == "mouse_up")
        return gLastEvent == int(SDL_MOUSEBUTTONUP);
    if (s == "window")
        return gLastEvent == int(SDL_WINDOWEVENT);
    return false;
}
void nexus_gfx_set_ui_resolution(double width, double height) {

    if (width <= 0.0 || height <= 0.0) {

        gUiWidth = 0.0;

        gUiHeight = 0.0;

    } else {

        gUiWidth = width;

        gUiHeight = height;
    }

    if (gGraphics) {
        if (gSoftware2D) {
            update_window_metrics();
            apply_software_layout();
        } else {
            apply_2d_viewport();
            setup_2d();
        }
    }
}

int64_t nexus_gfx_width() {
    return gWidth;
}
int64_t nexus_gfx_height() {
    return gHeight;
}
bool nexus_gfx_vsync(bool enabled) {
    if (!gGraphics)
        return false;
    if (gSoftware2D)
        return true;
    return SDL_GL_SetSwapInterval && SDL_GL_SetSwapInterval(enabled ? 1 : 0) == 0;
}
void nexus_gfx_set_title(const char* title) {
    if (gWindow && title && SDL_SetWindowTitle)
        SDL_SetWindowTitle(gWindow, title);
}

void nexus_gfx_fullscreen(bool enabled) {
    if (!gWindow || !SDL_SetWindowFullscreen)
        return;
    if (SDL_SetWindowFullscreen(gWindow, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0u) != 0) {
        gGraphicsError = std::string("fullscreen failed: ") + sdl_error();
    }
}

void nexus_gfx_maximize() {
    if (gWindow && SDL_MaximizeWindow)
        SDL_MaximizeWindow(gWindow);
}

void nexus_gfx_restore() {
    if (gWindow && SDL_RestoreWindow)
        SDL_RestoreWindow(gWindow);
}

bool nexus_gfx_is_fullscreen() {
    if (!gWindow || !SDL_GetWindowFlags)
        return false;
    return (SDL_GetWindowFlags(gWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

const char* nexus_gfx_backend() {
    if (!gGraphics)
        return "uninitialized";
    return gOpenGL3D ? "opengl-3d" : (gSoftware2D ? "sdl2-2d" : "unknown");
}
void nexus_gfx_begin() {
    if (!gGraphics)
        return;
    if (gSoftware2D) {
        update_window_metrics();
        apply_software_layout();
        return;
    }
    apply_2d_viewport();
    setup_2d();
}

void nexus_gfx_end() {
    if (gSoftware2D) {
        if (gRenderer)
            SDL_RenderPresent(gRenderer);
        return;
    }
    if (gWindow && SDL_GL_SwapWindow)
        SDL_GL_SwapWindow(gWindow);
}

void nexus_gfx_clear(double r, double g, double b, double a) {
    if (!gGraphics)
        return;
    if (gSoftware2D) {
        software_color(r, g, b, a);
        SDL_RenderClear(gRenderer);
        return;
    }
    glClearColor(float(r), float(g), float(b), float(a));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void nexus_gfx_rect(
    double x, double y, double w, double h, double r, double g, double b, double a) {
    if (!gGraphics)
        return;
    if (gSoftware2D) {
        software_color(r, g, b, a);
        software_fill_rect(x, y, w, h);
        return;
    }
    setup_2d();
    draw_quad(float(x), float(y), float(w), float(h), float(r), float(g), float(b), float(a));
}

void nexus_gfx_circle(double x, double y, double radius, double r, double g, double b, double a) {
    if (!gGraphics)
        return;
    if (gSoftware2D) {
        software_color(r, g, b, a);
        // Scale the scanline count with the radius: small chips and cursors do
        // not need 80 lines, and thousands of tiny circles are the usual
        // software-renderer bottleneck.
        int segments = int(std::clamp(radius * 1.6, 8.0, 40.0));
        for (int i = -segments; i <= segments; ++i) {
            double dy = radius * double(i) / double(segments);
            double half = std::sqrt(std::max(0.0, radius * radius - dy * dy));
            SDL_RenderDrawLine(gRenderer,
                               int(std::llround(x - half)),
                               int(std::llround(y + dy)),
                               int(std::llround(x + half)),
                               int(std::llround(y + dy)));
        }
        return;
    }
    setup_2d();
    draw_circle(float(x), float(y), float(radius), float(r), float(g), float(b), float(a));
}

void nexus_gfx_line(double x1,
                    double y1,
                    double x2,
                    double y2,
                    double width,
                    double r,
                    double g,
                    double b,
                    double a) {
    if (!gGraphics)
        return;
    if (gSoftware2D) {
        software_color(r, g, b, a);
        SDL_RenderDrawLine(gRenderer,
                           int(std::llround(x1)),
                           int(std::llround(y1)),
                           int(std::llround(x2)),
                           int(std::llround(y2)));
        for (int i = 1; i < int(std::floor(width)); ++i)
            SDL_RenderDrawLine(gRenderer,
                               int(std::llround(x1)) + i,
                               int(std::llround(y1)),
                               int(std::llround(x2)) + i,
                               int(std::llround(y2)));
        return;
    }
    setup_2d();
    glLineWidth(float(width));
    glColor4f(float(r), float(g), float(b), float(a));
    glBegin(GL_LINES);
    glVertex2f(float(x1), float(y1));
    glVertex2f(float(x2), float(y2));
    glEnd();
}

void nexus_gfx_text(
    double x, double y, double scale, const char* text, double r, double g, double b, double a) {
    if (!gGraphics || !text)
        return;
    if (gSoftware2D) {
        software_color(r, g, b, a);
        double px = x;
        double py = y;
        for (const char* p = text; *p; ++p) {
            if (*p == '\n') {
                px = x;
                py += 8.0 * scale;
                continue;
            }
            if (*p == ' ') {
                px += 6.0 * scale;
                continue;
            }
            const auto* bits = glyph(*p);
            if (bits) {
                for (int row = 0; row < 7; ++row)
                    for (int col = 0; col < 5; ++col)
                        if (bits[row] & (1u << (4 - col)))
                            software_fill_rect(px + col * scale, py + row * scale, scale, scale);
            }
            px += 6.0 * scale;
        }
        return;
    }
    setup_2d();
    draw_text(float(x), float(y), float(scale), text, float(r), float(g), float(b), float(a));
}

void nexus_gfx_set_fps(int64_t fps) {
    if (fps <= 0)
        return;
    /* language-level limiter is preferable; keep backend deterministic */
}

int64_t nexus_gfx_texture_load(const char* path) {

    if (!gGraphics || !path || !SDL_ConvertSurfaceFormat || !SDL_FreeSurface) {
        gGraphicsError = "graphics texture system is unavailable";
        return 0;
    }
    SDL_Surface* src = nullptr;

    if (IMG_Load)
        src = IMG_Load(path);

    if (!src && SDL_LoadBMP)
        src = SDL_LoadBMP(path);

    if (!src) {
        gGraphicsError = IMG_GetError ? std::string("image load failed: ") + IMG_GetError()
                                      : "image load failed (need SDL2_image for PNG/JPG or BMP)";
        return 0;
    }
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(src);
    if (!surf)
        return 0;
    const int sw = surf->w;
    const int sh = surf->h;
    if (gSoftware2D) {
        SDL_Texture* software = SDL_CreateTextureFromSurface(gRenderer, surf);
        SDL_FreeSurface(surf);
        if (!software) {
            gGraphicsError = std::string("SDL texture creation failed: ") + sdl_error();
            return 0;
        }
        SDL_SetTextureBlendMode(software, SDL_BLENDMODE_BLEND);
        const int64_t id = gNextTexture++;
        gTextures[id] = Texture{0, software, sw, sh};
        return id;
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    int64_t id = gNextTexture++;
    gTextures[id] = Texture{tex, nullptr, surf->w, surf->h};
    SDL_FreeSurface(surf);
    return id;
}
void nexus_gfx_texture_draw(
    int64_t id, double x, double y, double w, double h, double r, double g, double b, double a) {
    auto it = gTextures.find(id);
    if (!gGraphics || it == gTextures.end())
        return;
    if (gSoftware2D) {
        if (!it->second.software)
            return;
        SDL_Rect dst = software_rect(x, y, w, h);
        SDL_RenderCopy(gRenderer, it->second.software, nullptr, &dst);
        return;
    }
    setup_2d();
    draw_textured_quad(
        it->second, float(x), float(y), float(w), float(h), float(r), float(g), float(b), float(a));
}
void nexus_gfx_texture_unload(int64_t id) {
    auto it = gTextures.find(id);
    if (it == gTextures.end())
        return;
    if (it->second.id && glDeleteTextures && gOpenGL3D)
        glDeleteTextures(1, &it->second.id);
    if (it->second.software && SDL_DestroyTexture)
        SDL_DestroyTexture(it->second.software);
    gTextures.erase(it);
}
int64_t nexus_gfx_texture_width(int64_t id) {
    auto it = gTextures.find(id);
    return it == gTextures.end() ? 0 : it->second.w;
}
int64_t nexus_gfx_texture_height(int64_t id) {
    auto it = gTextures.find(id);
    return it == gTextures.end() ? 0 : it->second.h;
}
void nexus_gfx_texture_filter(int64_t id, bool nearest) {
    auto it = gTextures.find(id);
    if (it == gTextures.end() || !gGraphics)
        return;
    if (gSoftware2D) {
        // SDL2 software textures use nearest filtering implicitly.
        (void)nearest;
        return;
    }
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    GLint f = nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void nexus_gfx_camera_set(double x, double y, double zoom) {
    gCameraX = float(x);
    gCameraY = float(y);
    gCameraZoom = zoom > 0.01 ? float(zoom) : 0.01f;
}
void nexus_gfx_camera_reset() {
    gCameraX = 0;
    gCameraY = 0;
    gCameraZoom = 1;
}
void nexus_gfx_texture_draw_frame(int64_t id,
                                  double x,
                                  double y,
                                  double w,
                                  double h,
                                  int64_t frame,
                                  int64_t frameW,
                                  int64_t frameH,
                                  int64_t columns,
                                  double r,
                                  double g,
                                  double b,
                                  double a) {
    auto it = gTextures.find(id);
    if (!gGraphics || it == gTextures.end() || frameW <= 0 || frameH <= 0 || columns <= 0)
        return;
    const int64_t col = frame % columns;
    const int64_t row = frame / columns;
    if (gSoftware2D) {
        if (!it->second.software)
            return;
        SDL_Rect src{int(col * frameW), int(row * frameH), int(frameW), int(frameH)};
        SDL_Rect dst = software_rect(x, y, w, h);
        SDL_RenderCopy(gRenderer, it->second.software, &src, &dst);
        return;
    }
    const float u0 = float(col * frameW) / float(it->second.w);
    const float v0 = float(row * frameH) / float(it->second.h);
    const float u1 = float((col + 1) * frameW) / float(it->second.w);
    const float v1 = float((row + 1) * frameH) / float(it->second.h);
    setup_2d();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    glColor4f(float(r), float(g), float(b), float(a));
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex2f(float(x), float(y));
    glTexCoord2f(u1, v0);
    glVertex2f(float(x + w), float(y));
    glTexCoord2f(u1, v1);
    glVertex2f(float(x + w), float(y + h));
    glTexCoord2f(u0, v1);
    glVertex2f(float(x), float(y + h));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
int64_t nexus_gfx_anim_create(int64_t texture,
                              int64_t frameW,
                              int64_t frameH,
                              int64_t columns,
                              int64_t frameCount,
                              double fps,
                              bool loop) {

    auto it = gTextures.find(texture);

    if (it == gTextures.end() || frameW <= 0 || frameH <= 0 || columns <= 0 || frameCount <= 0 ||
        fps <= 0.0) {
        gGraphicsError = "invalid sprite animation parameters";
        return 0;
    }
    if (frameW > it->second.w || frameH > it->second.h) {
        gGraphicsError = "sprite frame exceeds texture dimensions";
        return 0;
    }
    SpriteAnim a;
    a.texture = texture;
    a.frameW = frameW;
    a.frameH = frameH;
    a.columns = columns;
    a.frameCount = frameCount;
    a.fps = fps;
    a.loop = loop;

    int64_t id = gNextAnim++;
    gAnims[id] = a;
    return id;
}
void nexus_gfx_anim_update(int64_t id, double dt) {
    auto it = gAnims.find(id);
    if (it == gAnims.end())
        return;
    auto& a = it->second;
    if (a.frameCount <= 1)
        return;
    a.time += std::max(dt, 0.0);
    double step = 1.0 / a.fps;
    while (a.time >= step) {
        a.time -= step;
        ++a.frame;
        if (a.frame >= a.frameCount) {
            if (a.loop)
                a.frame = 0;
            else
                a.frame = a.frameCount - 1;
        }
    }
}
void nexus_gfx_anim_draw(
    int64_t id, double x, double y, double w, double h, double r, double g, double b, double a) {
    auto it = gAnims.find(id);
    if (it == gAnims.end())
        return;
    const auto& v = it->second;
    nexus_gfx_texture_draw_frame(
        v.texture, x, y, w, h, v.frame, v.frameW, v.frameH, v.columns, r, g, b, a);
}
int64_t nexus_gfx_anim_frame(int64_t id) {
    auto it = gAnims.find(id);
    return it == gAnims.end() ? -1 : it->second.frame;
}
void nexus_gfx_anim_destroy(int64_t id) {
    gAnims.erase(id);
}

const char* nexus_gfx_image_type() {
    return IMG_Load ? "PNG/JPG/BMP (SDL2_image + SDL2 core)"
                    : "BMP only (install SDL2_image for PNG/JPG)";
}
int64_t nexus_gfx_font_load(const char* path, int64_t size) {
    load_optional_media_libs();
    if (!gTTF || !TTF_Init || !TTF_OpenFont || !TTF_CloseFont || !TTF_RenderUTF8_Blended ||
        size <= 0) {
        gGraphicsError = "TTF font backend unavailable; install SDL2_ttf";
        return 0;
    }
    if (!gTTFReady && TTF_Init() != 0) {
        gGraphicsError = TTF_GetError ? TTF_GetError() : "TTF_Init failed";
        return 0;
    }
    gTTFReady = true;
    TTF_Font* f = TTF_OpenFont(path, int(size));
    if (!f) {
        gGraphicsError = TTF_GetError ? TTF_GetError() : "font load failed";
        return 0;
    }
    int64_t id = gNextFont++;
    gFonts[id] = f;
    return id;
}
void nexus_gfx_text_font(
    int64_t id, double x, double y, const char* text, double r, double g, double b, double a) {
    auto it = gFonts.find(id);
    if (!gGraphics || it == gFonts.end() || !TTF_RenderUTF8_Blended)
        return;
    SDL_Color c{uint8_t(std::clamp(r, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(g, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(b, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(a, 0.0, 1.0) * 255.0)};
    SDL_Surface* s = TTF_RenderUTF8_Blended(it->second, text ? text : "", c);
    if (!s)
        return;
    if (gSoftware2D) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, s);
        if (texture) {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            SDL_Rect dst{int(std::llround(x)), int(std::llround(y)), s->w, s->h};
            SDL_RenderCopy(gRenderer, texture, nullptr, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(s);
        return;
    }
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(s);
    if (!surf)
        return;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    setup_2d();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(float(x), float(y));
    glTexCoord2f(1, 0);
    glVertex2f(float(x + surf->w), float(y));
    glTexCoord2f(1, 1);
    glVertex2f(float(x + surf->w), float(y + surf->h));
    glTexCoord2f(0, 1);
    glVertex2f(float(x), float(y + surf->h));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &tex);
    SDL_FreeSurface(surf);
}
void nexus_gfx_font_unload(int64_t id) {
    auto it = gFonts.find(id);
    if (it != gFonts.end()) {
        if (TTF_CloseFont)
            TTF_CloseFont(it->second);
        gFonts.erase(it);
    }
}

// Loads a font the user does not have to ship: try well-known system font
// files first and fall back to the runtime's own bundled font if present.
int64_t nexus_gfx_font_default(int64_t size) {
    load_optional_media_libs();
    if (!gTTF || !TTF_OpenFont || size <= 0) {
        gGraphicsError = "TTF font backend unavailable; install SDL2_ttf";
        return 0;
    }
    static const char* candidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
    };
    for (const char* path : candidates) {
        std::FILE* f = std::fopen(path, "rb");
        if (!f)
            continue;
        std::fclose(f);
        int64_t id = nexus_gfx_font_load(path, size);
        if (id != 0)
            return id;
    }
    gGraphicsError = "no system font found; pass an explicit .ttf path to gfx_font_load";
    return 0;
}
int64_t nexus_gfx_text_width(int64_t id, const char* text) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_SizeUTF8)
        return 0;
    int w = 0, h = 0;
    if (TTF_SizeUTF8(it->second, text ? text : "", &w, &h) != 0)
        return 0;
    return w;
}
int64_t nexus_gfx_text_height(int64_t id) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_FontHeight)
        return 0;
    return TTF_FontHeight(it->second);
}
// style: 1 = bold, 2 = italic, 3 = both, 0 = normal
void nexus_gfx_font_style(int64_t id, int64_t style) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_SetFontStyle)
        return;
    int flags = 0;
    if (style & 1)
        flags |= 1; // TTF_STYLE_BOLD
    if (style & 2)
        flags |= 2; // TTF_STYLE_ITALIC
    TTF_SetFontStyle(it->second, flags);
}
int64_t nexus_gfx_sound_load(const char* path);

void nexus_gfx_sound_play(int64_t id, bool loop);

void nexus_gfx_sound_stop();

void nexus_gfx_sound_volume(double v);

int64_t nexus_gfx_music_load(const char* path) {

    load_optional_media_libs();

    if (gMIX && Mix_OpenAudio && Mix_LoadMUS && Mix_FreeMusic && Mix_PlayMusic) {

        if (!gMixerReady && Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 2048) != 0) {
            gGraphicsError = Mix_GetError ? Mix_GetError() : "Mix_OpenAudio failed";
        } else {
            gMixerReady = true;
            Mix_Music* m = Mix_LoadMUS(path);
            if (m) {
                if (gMusic && Mix_FreeMusic)
                    Mix_FreeMusic(gMusic);
                gMusic = m;
                return 1;
            }
            gGraphicsError = Mix_GetError ? Mix_GetError() : "music load failed";
        }
    }
    // Guaranteed fallback: plain PCM WAV using the core SDL2 audio queue.
    return nexus_gfx_sound_load(path);
}
bool nexus_gfx_music_play(int64_t id, bool loop) {

    if (gMusic && Mix_PlayMusic)
        return Mix_PlayMusic(gMusic, loop ? -1 : 1) == 0;

    nexus_gfx_sound_play(id, loop);
    return gAudioDevice != 0;
}
void nexus_gfx_music_stop() {
    if (Mix_HaltMusic && gMusic)
        Mix_HaltMusic();
    nexus_gfx_sound_stop();
}
void nexus_gfx_music_volume(double v) {
    if (Mix_VolumeMusic && gMusic)
        Mix_VolumeMusic(std::clamp(int(v * 128.0), 0, 128));
    nexus_gfx_sound_volume(v);
}
void nexus_gfx_cursor_visible(bool visible) {
    const int requestedState = visible ? 1 : 0;
    if (SDL_ShowCursor && gCursorVisibleState != requestedState) {
        SDL_ShowCursor(requestedState);
        gCursorVisibleState = requestedState;
    }
}

static uint8_t canvas_u8(double v) {

    return uint8_t(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
}
static size_t canvas_index(const CanvasData& c, int x, int y) {

    return (size_t(y) * size_t(c.w) + size_t(x)) * 4u;
}
static void canvas_put(CanvasData& c, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

    if (x < 0 || y < 0 || x >= c.w || y >= c.h)
        return;

    const size_t i = canvas_index(c, x, y);

    c.pixels[i] = r;
    c.pixels[i + 1] = g;
    c.pixels[i + 2] = b;
    c.pixels[i + 3] = a;
}
static void canvas_circle(
    CanvasData& c, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

    if (radius <= 0) {
        canvas_put(c, cx, cy, r, g, b, a);
        return;
    }
    const int rr = radius * radius;

    for (int y = cy - radius; y <= cy + radius; ++y) {

        for (int x = cx - radius; x <= cx + radius; ++x) {

            const int dx = x - cx, dy = y - cy;

            if (dx * dx + dy * dy <= rr)
                canvas_put(c, x, y, r, g, b, a);
        }
    }
}
static void canvas_blit_texture(CanvasData& c) {
    if (gSoftware2D) {
        if (!c.software) {
            c.software = SDL_CreateTexture(
                gRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, c.w, c.h);
            if (c.software)
                SDL_SetTextureBlendMode(c.software, SDL_BLENDMODE_BLEND);
        }
        if (c.software)
            SDL_UpdateTexture(c.software, nullptr, c.pixels.data(), c.w * 4);
        return;
    }
    if (!c.tex)
        glGenTextures(1, &c.tex);
    glBindTexture(GL_TEXTURE_2D, c.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, c.w, c.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
static void canvas_checkpoint_internal(CanvasData& c) {

    c.undo.push_back(CanvasSnapshot{c.pixels});

    if (c.undo.size() > 32)
        c.undo.erase(c.undo.begin());

    c.redo.clear();
}
static bool canvas_save_bmp_impl(const CanvasData& c, const char* path) {

    if (!path || c.w <= 0 || c.h <= 0)
        return false;

    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;

    const uint32_t rowBytes = uint32_t(c.w * 4), pixelBytes = rowBytes * uint32_t(c.h),
                   fileSize = 54u + pixelBytes;

    auto w16 = [&](uint16_t v) {
        f.put(char(v & 0xff));
        f.put(char((v >> 8) & 0xff));
    };

    auto w32 = [&](uint32_t v) {
        f.put(char(v & 0xff));
        f.put(char((v >> 8) & 0xff));
        f.put(char((v >> 16) & 0xff));
        f.put(char((v >> 24) & 0xff));
    };

    w16(0x4D42);
    w32(fileSize);
    w16(0);
    w16(0);
    w32(54);
    w32(40);
    w32(uint32_t(c.w));
    w32(uint32_t(c.h));
    w16(1);
    w16(32);
    w32(0);
    w32(pixelBytes);
    w32(2835);
    w32(2835);
    w32(0);
    w32(0);

    for (int y = c.h - 1; y >= 0; --y) {

        for (int x = 0; x < c.w; ++x) {
            const size_t i = canvas_index(c, x, y);
            f.put(char(c.pixels[i + 2]));
            f.put(char(c.pixels[i + 1]));
            f.put(char(c.pixels[i]));
            f.put(char(c.pixels[i + 3]));
        }
    }
    return bool(f);
}
static bool canvas_load_bmp_impl(const char* path, CanvasData& out) {

    if (!path)
        return false;
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (b.size() < 54 || b[0] != 'B' || b[1] != 'M')
        return false;

    auto rd16v = [&](size_t o) -> uint16_t { return uint16_t(b[o]) | (uint16_t(b[o + 1]) << 8); };

    auto rd32v = [&](size_t o) -> uint32_t {
        return uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) |
               (uint32_t(b[o + 3]) << 24);
    };

    const uint32_t offset = rd32v(10);
    const int32_t w = int32_t(rd32v(18));
    const int32_t hs = int32_t(rd32v(22));
    const uint16_t bits = rd16v(28);
    const uint32_t comp = rd32v(30);

    if (w <= 0 || hs == 0 || (bits != 24 && bits != 32) || comp != 0 || offset >= b.size())
        return false;

    const int h = hs < 0 ? -hs : hs;
    const size_t pxBytes = size_t(bits / 8);
    const size_t rowStride = ((size_t(w) * pxBytes + 3u) / 4u) * 4u;

    if (size_t(h) * rowStride > b.size() - offset)
        return false;

    out.w = w;
    out.h = h;
    out.pixels.assign(size_t(w) * size_t(h) * 4u, 255);
    out.undo.clear();
    out.redo.clear();

    for (int y = 0; y < h; ++y) {
        const int sy = hs < 0 ? y : (h - 1 - y);
        const uint8_t* row = b.data() + offset + size_t(sy) * rowStride;
        for (int x = 0; x < w; ++x) {
            const uint8_t* px = row + size_t(x) * pxBytes;
            const size_t i = canvas_index(out, x, y);
            out.pixels[i] = px[2];
            out.pixels[i + 1] = px[1];
            out.pixels[i + 2] = px[0];
            out.pixels[i + 3] = (bits == 32 ? px[3] : 255);
        }
    }
    return true;
}
int64_t nexus_gfx_canvas_create(int64_t w, int64_t h, double r, double g, double b, double a) {

    if (!gGraphics || w <= 0 || h <= 0 || w > 8192 || h > 8192)
        return 0;

    CanvasData c;
    c.w = int(w);
    c.h = int(h);
    c.pixels.resize(size_t(c.w) * size_t(c.h) * 4u);

    const uint8_t R = canvas_u8(r), G = canvas_u8(g), B = canvas_u8(b), A = canvas_u8(a);

    for (size_t i = 0; i < c.pixels.size(); i += 4) {
        c.pixels[i] = R;
        c.pixels[i + 1] = G;
        c.pixels[i + 2] = B;
        c.pixels[i + 3] = A;
    }
    const int64_t id = gNextCanvas++;
    if (gSoftware2D) {
        c.software = SDL_CreateTexture(
            gRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, c.w, c.h);
        if (c.software)
            SDL_SetTextureBlendMode(c.software, SDL_BLENDMODE_BLEND);
    } else {
        glGenTextures(1, &c.tex);
        glBindTexture(GL_TEXTURE_2D, c.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, c.w, c.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c.pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    gCanvases[id] = std::move(c);
    return id;
}
void nexus_gfx_canvas_destroy(int64_t id) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    if (it->second.tex && glDeleteTextures && gOpenGL3D) {
        glDeleteTextures(1, &it->second.tex);
    }
    if (it->second.software && SDL_DestroyTexture) {
        SDL_DestroyTexture(it->second.software);
    }
    gCanvases.erase(it);
}
void nexus_gfx_canvas_clear(int64_t id, double r, double g, double b, double a) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    canvas_checkpoint_internal(it->second);
    const uint8_t R = canvas_u8(r), G = canvas_u8(g), B = canvas_u8(b), A = canvas_u8(a);
    for (size_t i = 0; i < it->second.pixels.size(); i += 4) {
        it->second.pixels[i] = R;
        it->second.pixels[i + 1] = G;
        it->second.pixels[i + 2] = B;
        it->second.pixels[i + 3] = A;
    }
}
void nexus_gfx_canvas_brush(
    int64_t id, double x, double y, double size, double r, double g, double b, double a) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    const int radius = std::max(1, int(std::round(size * 0.5)));
    canvas_circle(it->second,
                  int(std::round(x)),
                  int(std::round(y)),
                  radius,
                  canvas_u8(r),
                  canvas_u8(g),
                  canvas_u8(b),
                  canvas_u8(a));
}
void nexus_gfx_canvas_line(int64_t id,
                           double x1,
                           double y1,
                           double x2,
                           double y2,
                           double size,
                           double r,
                           double g,
                           double b,
                           double a) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    CanvasData& c = it->second;
    const double dx = x2 - x1, dy = y2 - y1;
    const int steps = std::max(1, int(std::ceil(std::max(std::abs(dx), std::abs(dy)))));
    for (int i = 0; i <= steps; ++i) {
        double t = double(i) / double(steps);
        canvas_circle(c,
                      int(std::round(x1 + dx * t)),
                      int(std::round(y1 + dy * t)),
                      std::max(1, int(std::round(size * 0.5))),
                      canvas_u8(r),
                      canvas_u8(g),
                      canvas_u8(b),
                      canvas_u8(a));
    }
}
void nexus_gfx_canvas_rect(int64_t id,
                           double x,
                           double y,
                           double w,
                           double h,
                           double size,
                           double r,
                           double g,
                           double b,
                           double a,
                           bool filled) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    CanvasData& c = it->second;
    int x0 = int(std::floor(x)), y0 = int(std::floor(y)), x1 = int(std::ceil(x + w)),
        y1 = int(std::ceil(y + h));
    if (filled) {
        x0 = std::max(0, x0);
        y0 = std::max(0, y0);
        x1 = std::min(c.w - 1, x1);
        y1 = std::min(c.h - 1, y1);
        for (int yy = y0; yy <= y1; ++yy)
            for (int xx = x0; xx <= x1; ++xx)
                canvas_put(c, xx, yy, canvas_u8(r), canvas_u8(g), canvas_u8(b), canvas_u8(a));
    } else {
        nexus_gfx_canvas_line(id, x0, y0, x1, y0, size, r, g, b, a);
        nexus_gfx_canvas_line(id, x1, y0, x1, y1, size, r, g, b, a);
        nexus_gfx_canvas_line(id, x1, y1, x0, y1, size, r, g, b, a);
        nexus_gfx_canvas_line(id, x0, y1, x0, y0, size, r, g, b, a);
    }
}
void nexus_gfx_canvas_fill(
    int64_t id, int64_t x, int64_t y, double r, double g, double b, double a) {

    auto it = gCanvases.find(id);
    if (it == gCanvases.end())
        return;
    CanvasData& c = it->second;
    if (x < 0 || y < 0 || x >= c.w || y >= c.h)
        return;
    canvas_checkpoint_internal(c);

    const size_t si = canvas_index(c, int(x), int(y));
    const uint8_t sr = c.pixels[si], sg = c.pixels[si + 1], sb = c.pixels[si + 2],
                  sa = c.pixels[si + 3];
    const uint8_t tr = canvas_u8(r), tg = canvas_u8(g), tb = canvas_u8(b), ta = canvas_u8(a);
    if (sr == tr && sg == tg && sb == tb && sa == ta)
        return;

    std::vector<int> stack;
    stack.push_back(int(y) * c.w + int(x));
    std::vector<uint8_t> seen(size_t(c.w) * size_t(c.h), 0);

    while (!stack.empty()) {
        int p = stack.back();
        stack.pop_back();
        if (p < 0 || p >= c.w * c.h || seen[size_t(p)])
            continue;
        int px = p % c.w, py = p / c.w;
        size_t i = canvas_index(c, px, py);
        if (c.pixels[i] != sr || c.pixels[i + 1] != sg || c.pixels[i + 2] != sb ||
            c.pixels[i + 3] != sa)
            continue;
        seen[size_t(p)] = 1;
        c.pixels[i] = tr;
        c.pixels[i + 1] = tg;
        c.pixels[i + 2] = tb;
        c.pixels[i + 3] = ta;
        if (px > 0)
            stack.push_back(p - 1);
        if (px + 1 < c.w)
            stack.push_back(p + 1);
        if (py > 0)
            stack.push_back(p - c.w);
        if (py + 1 < c.h)
            stack.push_back(p + c.w);
    }
}
void nexus_gfx_canvas_draw(int64_t id, double x, double y, double w, double h, double alpha) {
    auto it = gCanvases.find(id);
    if (!gGraphics || it == gCanvases.end())
        return;
    CanvasData& c = it->second;
    canvas_blit_texture(c);
    if (gSoftware2D) {
        if (c.software) {
            SDL_Rect dst = software_rect(x, y, w, h);
            SDL_RenderCopy(gRenderer, c.software, nullptr, &dst);
        }
        return;
    }
    setup_2d();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, c.tex);
    glColor4f(1, 1, 1, float(std::clamp(alpha, 0.0, 1.0)));
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(float(x), float(y));
    glTexCoord2f(1, 0);
    glVertex2f(float(x + w), float(y));
    glTexCoord2f(1, 1);
    glVertex2f(float(x + w), float(y + h));
    glTexCoord2f(0, 1);
    glVertex2f(float(x), float(y + h));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
bool nexus_gfx_canvas_save_bmp(int64_t id, const char* path) {
    auto it = gCanvases.find(id);
    return it != gCanvases.end() && canvas_save_bmp_impl(it->second, path);
}
int64_t nexus_gfx_canvas_load_bmp(const char* path) {
    if (!gGraphics)
        return 0;
    CanvasData c;
    if (!canvas_load_bmp_impl(path, c))
        return 0;
    if (gSoftware2D) {
        c.software = SDL_CreateTexture(
            gRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, c.w, c.h);
        if (c.software)
            SDL_SetTextureBlendMode(c.software, SDL_BLENDMODE_BLEND);
    } else {
        glGenTextures(1, &c.tex);
        glBindTexture(GL_TEXTURE_2D, c.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, c.w, c.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c.pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    const int64_t id = gNextCanvas++;
    gCanvases[id] = std::move(c);
    return id;
}
void nexus_gfx_canvas_checkpoint(int64_t id) {
    auto it = gCanvases.find(id);
    if (it != gCanvases.end())
        canvas_checkpoint_internal(it->second);
}
bool nexus_gfx_canvas_undo(int64_t id) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end() || it->second.undo.empty())
        return false;
    CanvasData& c = it->second;
    c.redo.push_back(CanvasSnapshot{c.pixels});
    c.pixels = std::move(c.undo.back().pixels);
    c.undo.pop_back();
    return true;
}
bool nexus_gfx_canvas_redo(int64_t id) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end() || it->second.redo.empty())
        return false;
    CanvasData& c = it->second;
    c.undo.push_back(CanvasSnapshot{c.pixels});
    c.pixels = std::move(c.redo.back().pixels);
    c.redo.pop_back();
    return true;
}
int64_t nexus_gfx_canvas_width(int64_t id) {
    auto it = gCanvases.find(id);
    return it == gCanvases.end() ? 0 : it->second.w;
}
int64_t nexus_gfx_canvas_height(int64_t id) {
    auto it = gCanvases.find(id);
    return it == gCanvases.end() ? 0 : it->second.h;
}


// UTF-8 byte-count-limited immediate-mode text input. The field has pointer
// focus, handles text events and Backspace by Unicode code point, and finishes
// editing on Enter or Tab. Selection/caret movement and IME editing commands
// are outside this minimal runtime widget.
char* nexus_ui_text_field(double x, double y, double w, double h,
                          const char* current, const char* placeholder, int64_t maxBytes) {
    std::string value = current ? current : "";
    auto safePrefix = [](const std::string& text, size_t limit) -> size_t {
        size_t end = std::min(limit, text.size());
        // If the cutoff is inside a UTF-8 code point, discard that whole code point.
        while (end > 0 && end < text.size() &&
               (static_cast<uint8_t>(text[end]) & 0xC0u) == 0x80u)
            --end;
        return end;
    };
    auto popCodePoint = [](std::string& text) {
        if (text.empty())
            return;
        size_t start = text.size() - 1;
        while (start > 0 &&
               (static_cast<uint8_t>(text[start]) & 0xC0u) == 0x80u)
            --start;
        text.resize(start);
    };

    const bool editable = maxBytes > 0;
    const size_t limit = editable
        ? static_cast<size_t>(std::clamp<int64_t>(maxBytes, 1, 8192))
        : std::max<size_t>(value.size(), 1);
    if (value.size() > limit)
        value.resize(safePrefix(value, limit));

    if (w <= 0.0 || h <= 0.0 || !gGraphics)
        return nx_strdup(value.c_str());

    const double mx = nexus_gfx_mouse_x();
    const double my = nexus_gfx_mouse_y();
    const bool hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    const bool sameField = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;

    if (nexus_gfx_mouse_clicked(1)) {
        if (hover && editable) {
            if (!sameField && SDL_StartTextInput)
                SDL_StartTextInput();
            gUiTextFieldFocused = true;
            gUiTextFieldX = x;
            gUiTextFieldY = y;
            gUiTextFieldWidth = w;
            gUiTextFieldHeight = h;
        } else if (sameField) {
            gUiTextFieldFocused = false;
            if (SDL_StopTextInput)
                SDL_StopTextInput();
            gTextInputThisFrame.clear();
        }
    }

    const bool focused = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;

    if (focused && editable) {
        // SDL_SCANCODE_BACKSPACE == 42.
        if (gKeyPressed.size() > 42 && gKeyPressed[42])
            popCodePoint(value);

        if (!gTextInputThisFrame.empty() && value.size() < limit) {
            const size_t allowed = limit - value.size();
            const size_t addBytes = safePrefix(gTextInputThisFrame, allowed);
            value.append(gTextInputThisFrame.data(), addBytes);
        }
        gTextInputThisFrame.clear();

        // Enter finishes editing; Tab releases this field for application navigation.
        if ((gKeyPressed.size() > 40 && gKeyPressed[40]) ||
            (gKeyPressed.size() > 43 && gKeyPressed[43])) {
            gUiTextFieldFocused = false;
            if (SDL_StopTextInput)
                SDL_StopTextInput();
        }
    }

    const bool currentlyFocused = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;
    const double bg = editable ? 0.035 : 0.025;
    nexus_gfx_rect(x, y, w, h, bg, bg + 0.018, bg + 0.035, 1.0);
    const double borderR = currentlyFocused ? 0.20 : (hover ? 0.20 : 0.12);
    const double borderG = currentlyFocused ? 0.72 : (hover ? 0.42 : 0.18);
    const double borderB = currentlyFocused ? 0.96 : (hover ? 0.58 : 0.28);
    const double thickness = currentlyFocused ? 2.0 : 1.0;
    nexus_gfx_rect(x, y, w, thickness, borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x, y + h - thickness, w, thickness, borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x, y + thickness, thickness, std::max(0.0, h - 2.0 * thickness),
                    borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x + w - thickness, y + thickness, thickness,
                    std::max(0.0, h - 2.0 * thickness), borderR, borderG, borderB, 1.0);

    const size_t visibleBudget = w > 28.0 ? static_cast<size_t>((w - 24.0) / 9.0) : 0u;
    std::string visible = value;
    if (visible.size() > visibleBudget) {
        size_t start = visible.size() - visibleBudget;
        while (start < visible.size() &&
               (static_cast<uint8_t>(visible[start]) & 0xC0u) == 0x80u)
            ++start;
        visible = visible.substr(start);
    }

    const bool empty = value.empty();
    const char* displayed = empty ? (placeholder ? placeholder : "") : visible.c_str();
    const double textY = y + std::max(3.0, (h - 19.0) * 0.5);
    if (empty)
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.43, 0.51, 0.62, 1.0);
    else
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.91, 0.95, 1.0, 1.0);

    if (currentlyFocused && editable && std::fmod(now_seconds(), 1.0) < 0.55) {
        const double cursorOffset = std::min(std::max(0.0, w - 22.0),
                                             static_cast<double>(visible.size()) * 9.0);
        nexus_gfx_rect(x + 10.0 + cursorOffset, y + 8.0, 1.5,
                       std::max(2.0, h - 16.0), 0.35, 0.84, 1.0, 1.0);
    }

    return nx_strdup(value.c_str());
}

bool nexus_ui_button(double x, double y, double w, double h, const char* label) {
    if (w <= 0.0 || h <= 0.0)
        return false;
    bool hover = false;
    if (gGraphics) {
        const double mx = nexus_gfx_mouse_x();
        const double my = nexus_gfx_mouse_y();
        hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
    nexus_gfx_rect(x, y, w, h, hover ? 0.12 : 0.08, hover ? 0.48 : 0.32, 0.82, 1.0);
    nexus_gfx_text(x + 12, y + 12, 1.5, label ? label : "", 1, 1, 1, 1);
    // Buttons activate only on a left-button press edge, not right/middle clicks
    // and not on every frame while the mouse button remains held.
    return hover && nexus_gfx_mouse_clicked(1);
}
void nexus_ui_panel(
    double x, double y, double w, double h, double r, double g, double b, double a) {
    nexus_gfx_rect(x, y, w, h, r, g, b, a);
}
void nexus_ui_label(double x, double y, const char* text, double scale) {
    nexus_gfx_text(x, y, scale, text, 1, 1, 1, 1);
}
void nexus_ui_progress(
    double x, double y, double w, double h, double value, double r, double g, double b, double a) {
    nexus_gfx_rect(x, y, w, h, 0.12, 0.14, 0.18, 1);
    double v = std::clamp(value, 0.0, 1.0);
    nexus_gfx_rect(x, y, w * v, h, r, g, b, a);
}
bool nexus_ui_checkbox(double x, double y, const char* label, bool checked) {
    bool hover = false;
    if (gGraphics) {
        const double mx = nexus_gfx_mouse_x();
        const double my = nexus_gfx_mouse_y();
        // The caption is part of the hit target, so users do not need to hit
        // the small square precisely.
        const double labelWidth = label ? static_cast<double>(std::strlen(label)) * 10.0 : 0.0;
        hover = mx >= x && mx <= x + 40.0 + labelWidth && my >= y && my <= y + 28.0;
    }
    bool on = checked;
    if (hover && nexus_gfx_mouse_clicked(1))
        on = !checked;
    nexus_gfx_rect(x, y, 28, 28, 0.10, 0.12, 0.18, 1.0);
    if (on)
        nexus_gfx_rect(x + 5, y + 5, 18, 18, 0.2, 0.75, 1.0, 1.0);
    if (label)
        nexus_gfx_text(x + 40, y + 2, 1.4, label, 1, 1, 1, 1);
    return on;
}
double nexus_ui_slider(double x, double y, double w, double h, double value) {
    double v = std::clamp(value, 0.0, 1.0);
    if (w <= 0.0 || h <= 0.0)
        return v;

    double mx = 0.0;
    double my = 0.0;
    bool down = false;
    if (gGraphics) {
        mx = nexus_gfx_mouse_x();
        my = nexus_gfx_mouse_y();
        down = nexus_gfx_mouse_down(1);
    }
    const bool hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    if (!down)
        gUiSliderDragging = false;
    if (nexus_gfx_mouse_clicked(1) && hover) {
        gUiSliderDragging = true;
        gUiSliderX = x;
        gUiSliderY = y;
        gUiSliderWidth = w;
        gUiSliderHeight = h;
    }

    // Capture the slider from mouse-down until mouse-up. Once dragging starts,
    // keep updating it even if the cursor leaves the original track.
    const bool isActiveSlider = gUiSliderDragging &&
        gUiSliderX == x && gUiSliderY == y &&
        gUiSliderWidth == w && gUiSliderHeight == h;
    if (down && isActiveSlider)
        v = std::clamp((mx - x) / w, 0.0, 1.0);

    nexus_gfx_rect(x, y + h * 0.35, w, h * 0.3, 0.12, 0.14, 0.18, 1.0);
    nexus_gfx_rect(x, y + h * 0.35, w * v, h * 0.3, 0.2, 0.65, 1.0, 1.0);
    nexus_gfx_circle(x + w * v, y + h * 0.5, h * 0.7, 0.9, 0.95, 1.0, 1.0);
    return v;
}
bool nexus_physics_aabb(
    double ax, double ay, double aw, double ah, double bx, double by, double bw, double bh) {
    if (aw <= 0.0 || ah <= 0.0 || bw <= 0.0 || bh <= 0.0)
        return false;
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
bool nexus_physics_circle(double ax, double ay, double ar, double bx, double by, double br) {
    if (ar < 0.0 || br < 0.0)
        return false;
    double dx = ax - bx, dy = ay - by, r = ar + br;
    return dx * dx + dy * dy <= r * r;
}
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
int64_t nexus_ecs_create() {
    int64_t id = gNextEntity++;
    gEntities[id] = EcsEntity{};
    return id;
}
void nexus_ecs_destroy(int64_t id) {
    gEntities.erase(id);
}
bool nexus_ecs_alive(int64_t id) {
    return gEntities.contains(id);
}
void nexus_ecs_set_position(int64_t id, double x, double y) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.x = float(x);
        it->second.y = float(y);
    }
}
void nexus_ecs_set_velocity(int64_t id, double x, double y) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.vx = float(x);
        it->second.vy = float(y);
    }
}
void nexus_ecs_set_size(int64_t id, double w, double h) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.w = float(w);
        it->second.h = float(h);
    }
}
void nexus_ecs_set_color(int64_t id, double r, double g, double b, double a) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.r = float(r);
        it->second.g = float(g);
        it->second.b = float(b);
        it->second.a = float(a);
    }
}
void nexus_ecs_set_texture(int64_t id, int64_t tex) {
    auto it = gEntities.find(id);
    if (it != gEntities.end())
        it->second.texture = tex;
}
void nexus_ecs_update(double dt) {
    for (auto& [id, e] : gEntities) {
        if (e.alive) {
            e.x += e.vx * float(dt);
            e.y += e.vy * float(dt);
        }
    }
}
void nexus_ecs_draw() {
    if (!gGraphics)
        return;
    for (auto& [id, e] : gEntities) {
        if (!e.alive)
            continue;
        if (e.texture)
            nexus_gfx_texture_draw(e.texture, e.x, e.y, e.w, e.h, e.r, e.g, e.b, e.a);
        else
            nexus_gfx_rect(e.x, e.y, e.w, e.h, e.r, e.g, e.b, e.a);
    }
}
bool nexus_ecs_collides(int64_t a, int64_t b) {
    auto ia = gEntities.find(a), ib = gEntities.find(b);
    if (ia == gEntities.end() || ib == gEntities.end())
        return false;
    const auto& A = ia->second;
    const auto& B = ib->second;
    return A.x < B.x + B.w && A.x + A.w > B.x && A.y < B.y + B.h && A.y + A.h > B.y;
}
double nexus_ecs_x(int64_t id) {
    auto it = gEntities.find(id);
    return it == gEntities.end() ? 0 : it->second.x;
}
double nexus_ecs_y(int64_t id) {
    auto it = gEntities.find(id);
    return it == gEntities.end() ? 0 : it->second.y;
}
int64_t nexus_ecs_count() {
    return int64_t(gEntities.size());
}

// Simple OpenGL 2.1 compatible 3D backend.
void nexus_gfx3d_begin(double fov,
                       double nearPlane,
                       double farPlane,
                       double camX,
                       double camY,
                       double camZ,
                       double pitch,
                       double yaw,
                       double roll) {
    if (!gGraphics)
        return;
    if (!gOpenGL3D && !ensure_opengl_backend())
        return;
    glViewport(0, 0, gWidth, gHeight);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = double(gWidth) / double(gHeight ? gHeight : 1);
    double top = nearPlane * std::tan(fov * 3.14159265358979323846 / 360.0);
    double right = top * aspect;
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(float(-roll), 0, 0, 1);
    glRotatef(float(-pitch), 1, 0, 0);
    glRotatef(float(-yaw), 0, 1, 0);
    glTranslatef(float(-camX), float(-camY), float(-camZ));
}
void nexus_gfx3d_cube(double x,
                      double y,
                      double z,
                      double sx,
                      double sy,
                      double sz,
                      double r,
                      double g,
                      double b,
                      double a) {
    if (!gGraphics)
        return;
    glColor4f(float(r), float(g), float(b), float(a));
    float x0 = float(x - sx / 2), x1 = float(x + sx / 2), y0 = float(y - sy / 2),
          y1 = float(y + sy / 2), z0 = float(z - sz / 2), z1 = float(z + sz / 2);
    glBegin(GL_QUADS);

    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);
    glEnd();
}
void nexus_gfx3d_grid(int64_t cells, double spacing, double r, double g, double b, double a) {
    if (!gGraphics)
        return;
    glColor4f(float(r), float(g), float(b), float(a));
    glLineWidth(1);
    glBegin(GL_LINES);
    for (int64_t i = -cells; i <= cells; i++) {
        float p = float(i) * float(spacing);
        glVertex3f(p, 0, float(-cells * spacing));
        glVertex3f(p, 0, float(cells * spacing));
        glVertex3f(float(-cells * spacing), 0, p);
        glVertex3f(float(cells * spacing), 0, p);
    }
    glEnd();
}
void nexus_gfx3d_end() {
    if (!gOpenGL3D || !gWindow || !SDL_GL_SwapWindow)
        return;
    SDL_GL_SwapWindow(gWindow);
}

// Lightweight OBJ model loader for the fixed-function 3D backend.
int64_t nexus_gfx3d_model_load(const char* path) {

    if (!path) {
        gGraphicsError = "OBJ path is null";
        return 0;
    }
    std::ifstream f(path);
    if (!f) {
        gGraphicsError = std::string("cannot open OBJ: ") + path;
        return 0;
    }
    std::vector<std::array<float, 3>> pos;
    Model3D model;
    std::string line;

    while (std::getline(f, line)) {

        std::istringstream in(line);
        std::string tag;
        in >> tag;

        if (tag == "v") {
            float x, y, z;
            if (in >> x >> y >> z)
                pos.push_back({x, y, z});
        } else if (tag == "f") {

            std::vector<int> ids;
            std::string tok;

            while (in >> tok) {
                auto slash = tok.find('/');
                std::string n = tok.substr(0, slash);
                try {
                    int idx = std::stoi(n);
                    if (idx < 0)
                        idx = int(pos.size()) + idx + 1;
                    if (idx > 0 && idx <= int(pos.size()))
                        ids.push_back(idx - 1);
                } catch (...) {}
            }
            if (ids.size() >= 3) {
                for (size_t i = 1; i + 1 < ids.size(); ++i) {
                    for (int k : {ids[0], ids[i], ids[i + 1]}) {
                        auto v = pos[size_t(k)];
                        model.vertices.push_back(v[0]);
                        model.vertices.push_back(v[1]);
                        model.vertices.push_back(v[2]);
                    }
                }
            }
        }
    }
    if (model.vertices.empty()) {
        gGraphicsError = "OBJ contains no triangle faces";
        return 0;
    }
    int64_t id = gNextModel++;
    gModels[id] = std::move(model);
    return id;
}
void nexus_gfx3d_model_draw(int64_t id,
                            double x,
                            double y,
                            double z,
                            double sx,
                            double sy,
                            double sz,
                            double rx,
                            double ry,
                            double rz) {

    auto it = gModels.find(id);
    if (!gGraphics || it == gModels.end())
        return;

    glPushMatrix();
    glTranslatef(float(x), float(y), float(z));
    glRotatef(float(rx), 1, 0, 0);
    glRotatef(float(ry), 0, 1, 0);
    glRotatef(float(rz), 0, 0, 1);
    glScalef(float(sx), float(sy), float(sz));
    glColor4f(0.75f, 0.85f, 1.0f, 1.0f);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i + 2 < it->second.vertices.size(); i += 3)
        glVertex3f(it->second.vertices[i], it->second.vertices[i + 1], it->second.vertices[i + 2]);
    glEnd();
    glPopMatrix();
}
void nexus_gfx3d_model_unload(int64_t id) {
    gModels.erase(id);
}

int64_t nexus_gfx_particle_create(double x,
                                  double y,
                                  double vx,
                                  double vy,
                                  double life,
                                  double size,
                                  double r,
                                  double g,
                                  double b,
                                  double a) {
    Particle p;
    p.x = float(x);
    p.y = float(y);
    p.vx = float(vx);
    p.vy = float(vy);
    p.life = float(std::max(life, 0.001));
    p.maxLife = p.life;
    p.size = float(size);
    p.r = float(r);
    p.g = float(g);
    p.b = float(b);
    p.a = float(a);
    int64_t id = gNextParticle++;
    gParticles[id] = p;
    return id;
}
bool nexus_gfx_particle_alive(int64_t id) {
    auto it = gParticles.find(id);
    return it != gParticles.end() && it->second.alive;
}
void nexus_gfx_particles_update(double dt) {
    float d = float(std::max(dt, 0.0));
    for (auto& [id, p] : gParticles) {
        if (!p.alive)
            continue;
        p.x += p.vx * d;
        p.y += p.vy * d;
        p.life -= d;
        if (p.life <= 0)
            p.alive = false;
    }
}
void nexus_gfx_particles_draw() {
    for (const auto& [id, p] : gParticles) {
        if (!p.alive)
            continue;
        float fade = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        draw_circle(p.x, p.y, p.size, p.r, p.g, p.b, p.a * fade);
    }
}
void nexus_gfx_particle_destroy(int64_t id) {
    gParticles.erase(id);
}
void nexus_gfx_particles_clear() {
    gParticles.clear();
}

// WAV audio using SDL2 core audio queue. No SDL2_mixer or SDL_LoadWAV symbol is required.
int64_t nexus_gfx_sound_load(const char* path) {

    gAudioError.clear();

    if (!load_sdl() || !path || !SDL_OpenAudioDevice || !SDL_CloseAudioDevice ||
        !SDL_PauseAudioDevice || !SDL_QueueAudio || !SDL_GetQueuedAudioSize) {
        gAudioError = "SDL2 core audio API unavailable";
        gGraphicsError = gAudioError;
        return 0;
    }
    if (SDL_Init(0x00000010u) != 0) {
        gAudioError = std::string("SDL audio init failed: ") + sdl_error();
        gGraphicsError = gAudioError;
        return 0;
    }
    SDL_AudioSpec source{};
    std::vector<uint8_t> raw;
    std::string err;

    if (!load_pcm_wav(path, source, raw, err)) {
        gAudioError = err;
        gGraphicsError = err;
        return 0;
    }
    if (!ensure_audio_device(source)) {
        gGraphicsError = gAudioError;
        return 0;
    }
    std::vector<uint8_t> converted = convert_pcm(raw, source, gAudioSpec);

    if (converted.empty()) {
        gAudioError = "failed to convert WAV to the active audio device format";
        gGraphicsError = gAudioError;
        return 0;
    }
    Sound s;
    s.data = std::move(converted);
    s.freq = gAudioSpec.freq;
    s.format = gAudioSpec.format;
    s.channels = gAudioSpec.channels;
    s.samples = gAudioSpec.samples;

    int64_t id = gNextSound++;
    gSounds[id] = std::move(s);
    gAudio = true;
    return id;
}

void nexus_gfx_sound_tone(double frequency, int64_t durationMs, double volume) {

    gAudioError.clear();

    if (!load_sdl() || !SDL_OpenAudioDevice || !SDL_CloseAudioDevice || !SDL_PauseAudioDevice ||
        !SDL_QueueAudio) {
        gAudioError = "SDL2 audio queue API unavailable";
        return;
    }
    if (SDL_Init(0x00000010u) != 0) {
        gAudioError = std::string("SDL audio init failed: ") + sdl_error();
        return;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = 48000;
    wanted.format = 0x8010u;
    wanted.channels = 2;
    wanted.samples = 1024;

    if (!ensure_audio_device(wanted)) {
        return;
    }
    const double freq = std::max(20.0, std::min(20000.0, frequency));

    const int ms = std::max<int64_t>(8, std::min<int64_t>(durationMs, 2000));

    const int frames = int((int64_t(gAudioSpec.freq) * ms) / 1000);

    const int channels = std::max<int>(1, gAudioSpec.channels);

    const int bytes = (gAudioSpec.format == 0x0008u) ? 1 : (gAudioSpec.format == 0x8010u ? 2 : 0);

    if (bytes == 0) {
        gAudioError = "unsupported active audio format";
        return;
    }
    std::vector<uint8_t> data(size_t(frames) * size_t(channels) * size_t(bytes));

    double amp = std::clamp(volume, 0.0, 1.0);

    for (int i = 0; i < frames; ++i) {

        double t = double(i) / double(gAudioSpec.freq);

        double fadeIn = std::min(1.0, t / 0.006);

        double fadeOut = std::min(1.0, double(frames - i) / double(gAudioSpec.freq) * 18.0);

        double sample = std::sin(2.0 * 3.141592653589793 * freq * t) * amp * fadeIn * fadeOut;

        for (int c = 0; c < channels; ++c) {

            size_t at = (size_t(i) * size_t(channels) + size_t(c)) * size_t(bytes);

            if (bytes == 2) {
                int v = int(std::lrint(sample * 32767.0));
                data[at] = uint8_t(v & 0xff);
                data[at + 1] = uint8_t((uint16_t(v) >> 8) & 0xff);
            } else
                data[at] = uint8_t(std::clamp(128 + int(std::lrint(sample * 127.0)), 0, 255));
        }
    }
    if (SDL_ClearQueuedAudio)
        SDL_ClearQueuedAudio(gAudioDevice);

    if (SDL_QueueAudio(gAudioDevice, data.data(), uint32_t(data.size())) != 0)
        gAudioError = "SDL_QueueAudio failed";

    else
        gAudio = true;
}
void nexus_gfx_sound_play(int64_t id, bool loop) {

    auto it = gSounds.find(id);
    if (it == gSounds.end()) {
        gAudioError = "unknown sound id";
        return;
    }
    if (!gAudioDevice) {
        gAudioError = "audio device is not open";
        return;
    }
    const Sound& s = it->second;

    std::vector<uint8_t> scaled = s.data;

    if (gAudioVolume < 128) {

        if (s.format == 0x8010u) {

            for (size_t i = 0; i + 1 < scaled.size(); i += 2) {
                int16_t sample = int16_t(uint16_t(scaled[i]) | (uint16_t(scaled[i + 1]) << 8));
                sample = int16_t(std::clamp(int(sample) * gAudioVolume / 128, -32768, 32767));
                scaled[i] = uint8_t(sample & 0xff);
                scaled[i + 1] = uint8_t((uint16_t(sample) >> 8) & 0xff);
            }
        } else if (s.format == 0x0008u) {

            for (auto& v : scaled) {
                int sample = int(v) - 128;
                sample = std::clamp(sample * gAudioVolume / 128, -128, 127);
                v = uint8_t(sample + 128);
            }
        }
    }
    if (!SDL_QueueAudio(gAudioDevice, scaled.data(), uint32_t(scaled.size()))) {
        gAudioError = "SDL_QueueAudio failed";
        gGraphicsError = gAudioError;
        return;
    }
    if (loop) {
        for (int i = 0; i < 63; i++)
            SDL_QueueAudio(gAudioDevice, scaled.data(), uint32_t(scaled.size()));
    }
}
void nexus_gfx_sound_stop() {
    if (gAudioDevice) {
        if (SDL_ClearQueuedAudio)
            SDL_ClearQueuedAudio(gAudioDevice);
        SDL_CloseAudioDevice(gAudioDevice);
        gAudioDevice = 0;
    }
    gAudio = false;
}
void nexus_gfx_sound_unload(int64_t id) {
    gSounds.erase(id);
}
int64_t nexus_gfx_audio_available() {
    return (gAudioDevice != 0 || load_sdl()) ? 1 : 0;
}
const char* nexus_gfx_audio_error() {
    return gAudioError.empty() ? "" : gAudioError.c_str();
}
void nexus_gfx_sound_volume(double v) {
    gAudioVolume = std::clamp(int(v * 128.0), 0, 128);
}
bool nexus_gfx_sound_playing() {
    return gAudioDevice && SDL_GetQueuedAudioSize && SDL_GetQueuedAudioSize(gAudioDevice) > 0;
}

} // extern C
