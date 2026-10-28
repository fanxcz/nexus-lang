
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
