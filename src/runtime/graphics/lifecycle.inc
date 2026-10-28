
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
