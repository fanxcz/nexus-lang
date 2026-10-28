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
