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
