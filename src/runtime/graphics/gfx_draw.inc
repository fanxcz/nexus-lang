
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
