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

bool nexus_gfx_canvas_blit(int64_t destinationId, int64_t sourceId) {
    auto destination = gCanvases.find(destinationId);
    auto source = gCanvases.find(sourceId);
    if (destination == gCanvases.end() || source == gCanvases.end() ||
        destinationId == sourceId) {
        gGraphicsError = "canvas blit requires two different live canvases";
        return false;
    }
    if (!canvas_blit_pixels(destination->second, source->second)) {
        gGraphicsError = "canvas blit requires canvases with matching dimensions";
        return false;
    }
    return true;
}

bool nexus_gfx_canvas_import_image(int64_t id, const char* path) {
    auto canvas = gCanvases.find(id);
    if (canvas == gCanvases.end() || !path || !*path) {
        gGraphicsError = "canvas image import requires a live canvas and a file path";
        return false;
    }
    if (!SDL_FreeSurface || !SDL_ConvertSurfaceFormat) {
        gGraphicsError = "SDL surface conversion is unavailable";
        return false;
    }

    SDL_Surface* source = IMG_Load ? IMG_Load(path) : nullptr;
    if (!source && SDL_LoadBMP)
        source = SDL_LoadBMP(path);
    if (!source) {
        gGraphicsError = IMG_GetError
            ? std::string("image import failed: ") + IMG_GetError()
            : "image import failed (SDL2_image or a valid BMP file is required)";
        return false;
    }

    if (source->w <= 0 || source->h <= 0 || source->w > 8192 || source->h > 8192) {
        SDL_FreeSurface(source);
        gGraphicsError = "image dimensions must be between 1 and 8192 pixels";
        return false;
    }

    SDL_Surface* converted =
        SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(source);
    if (!converted) {
        gGraphicsError = std::string("image conversion failed: ") + sdl_error();
        return false;
    }

    bool locked = false;
    bool imported = false;
    if (converted->pixels && converted->pitch >= converted->w * 4 && SDL_LockSurface) {
        if (SDL_LockSurface(converted) == 0) {
            locked = true;
            imported = canvas_import_pixels(canvas->second,
                                            static_cast<const uint8_t*>(converted->pixels),
                                            converted->w,
                                            converted->h,
                                            converted->pitch);
        } else {
            gGraphicsError = std::string("image surface lock failed: ") + sdl_error();
        }
    } else if (converted->pixels && converted->pitch >= converted->w * 4) {
        imported = canvas_import_pixels(canvas->second,
                                        static_cast<const uint8_t*>(converted->pixels),
                                        converted->w,
                                        converted->h,
                                        converted->pitch);
    } else {
        gGraphicsError = "converted image surface has invalid pixel storage";
    }
    if (locked && SDL_UnlockSurface)
        SDL_UnlockSurface(converted);
    SDL_FreeSurface(converted);

    if (!imported && gGraphicsError.empty())
        gGraphicsError = "image pixels could not be imported into the canvas";
    if (imported)
        gGraphicsError.clear();
    return imported;
}

bool nexus_gfx_canvas_save_png(int64_t id, const char* path) {
    auto it = gCanvases.find(id);
    if (it == gCanvases.end() || !path || !*path) {
        gGraphicsError = "PNG export requires a live canvas and a file path";
        return false;
    }
    if (!IMG_SavePNG || !SDL_CreateRGBSurfaceWithFormat || !SDL_FreeSurface) {
        gGraphicsError = "PNG export requires SDL2_image with IMG_SavePNG support";
        return false;
    }

    CanvasData& canvas = it->second;
    SDL_Surface* surface =
        SDL_CreateRGBSurfaceWithFormat(0, canvas.w, canvas.h, 32, SDL_PIXELFORMAT_ABGR8888);
    if (!surface) {
        gGraphicsError = std::string("PNG surface creation failed: ") + sdl_error();
        return false;
    }
    if (!surface->pixels || surface->pitch < canvas.w * 4) {
        SDL_FreeSurface(surface);
        gGraphicsError = "PNG surface has invalid pixel storage";
        return false;
    }

    bool locked = false;
    if (SDL_LockSurface) {
        if (SDL_LockSurface(surface) != 0) {
            gGraphicsError = std::string("PNG surface lock failed: ") + sdl_error();
            SDL_FreeSurface(surface);
            return false;
        }
        locked = true;
    }
    for (int y = 0; y < canvas.h; ++y) {
        std::memcpy(static_cast<uint8_t*>(surface->pixels) + size_t(y) * size_t(surface->pitch),
                    canvas.pixels.data() + size_t(y) * size_t(canvas.w) * 4u,
                    size_t(canvas.w) * 4u);
    }
    if (locked && SDL_UnlockSurface)
        SDL_UnlockSurface(surface);

    const int result = IMG_SavePNG(surface, path);
    SDL_FreeSurface(surface);
    if (result != 0) {
        gGraphicsError = IMG_GetError
            ? std::string("PNG export failed: ") + IMG_GetError()
            : "PNG export failed";
        return false;
    }
    gGraphicsError.clear();
    return true;
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

#ifdef NEXUS_RUNTIME_TESTING
int nexus_runtime_test_canvas_blend_pixel(const uint8_t* destination,
                                          const uint8_t* source,
                                          uint8_t* result) {
    if (!destination || !source || !result)
        return 0;
    std::memcpy(result, destination, 4);
    canvas_blend_pixel(result, source);
    return 1;
}
int nexus_runtime_test_canvas_import_rgba(int64_t destinationWidth,
                                          int64_t destinationHeight,
                                          const uint8_t* destinationPixels,
                                          const uint8_t* sourcePixels,
                                          int64_t sourceWidth,
                                          int64_t sourceHeight,
                                          int64_t sourcePitch,
                                          uint8_t* resultPixels) {
    if (!destinationPixels || !sourcePixels || !resultPixels ||
        destinationWidth < 1 || destinationHeight < 1 ||
        destinationWidth > 8192 || destinationHeight > 8192 ||
        sourceWidth < 1 || sourceHeight < 1 || sourceWidth > 8192 || sourceHeight > 8192 ||
        sourcePitch < sourceWidth * 4 || sourcePitch > std::numeric_limits<int>::max())
        return 0;
    CanvasData destination;
    destination.w = static_cast<int>(destinationWidth);
    destination.h = static_cast<int>(destinationHeight);
    const size_t destinationBytes =
        size_t(destination.w) * size_t(destination.h) * 4u;
    destination.pixels.assign(destinationPixels, destinationPixels + destinationBytes);
    if (!canvas_import_pixels(destination,
                              sourcePixels,
                              static_cast<int>(sourceWidth),
                              static_cast<int>(sourceHeight),
                              static_cast<int>(sourcePitch)))
        return 0;
    std::memcpy(resultPixels, destination.pixels.data(), destinationBytes);
    return 1;
}
int nexus_runtime_test_canvas_blit_rgba(int64_t width,
                                        int64_t height,
                                        const uint8_t* destinationPixels,
                                        const uint8_t* sourcePixels,
                                        uint8_t* resultPixels) {
    if (!destinationPixels || !sourcePixels || !resultPixels ||
        width < 1 || height < 1 || width > 8192 || height > 8192)
        return 0;
    CanvasData destination, source;
    destination.w = source.w = static_cast<int>(width);
    destination.h = source.h = static_cast<int>(height);
    const size_t bytes = size_t(destination.w) * size_t(destination.h) * 4u;
    destination.pixels.assign(destinationPixels, destinationPixels + bytes);
    source.pixels.assign(sourcePixels, sourcePixels + bytes);
    if (!canvas_blit_pixels(destination, source, false))
        return 0;
    std::memcpy(resultPixels, destination.pixels.data(), bytes);
    return 1;
}
#endif
