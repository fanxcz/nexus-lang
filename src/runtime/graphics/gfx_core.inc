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
