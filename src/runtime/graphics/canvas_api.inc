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
