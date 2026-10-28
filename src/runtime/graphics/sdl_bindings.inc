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
