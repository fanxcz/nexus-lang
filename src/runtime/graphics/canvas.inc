
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
