
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
