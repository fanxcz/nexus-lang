
int64_t nexus_gfx_texture_load(const char* path) {

    if (!gGraphics || !path || !SDL_ConvertSurfaceFormat || !SDL_FreeSurface) {
        gGraphicsError = "graphics texture system is unavailable";
        return 0;
    }
    SDL_Surface* src = nullptr;

    if (IMG_Load)
        src = IMG_Load(path);

    if (!src && SDL_LoadBMP)
        src = SDL_LoadBMP(path);

    if (!src) {
        gGraphicsError = IMG_GetError ? std::string("image load failed: ") + IMG_GetError()
                                      : "image load failed (need SDL2_image for PNG/JPG or BMP)";
        return 0;
    }
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(src);
    if (!surf)
        return 0;
    const int sw = surf->w;
    const int sh = surf->h;
    if (gSoftware2D) {
        SDL_Texture* software = SDL_CreateTextureFromSurface(gRenderer, surf);
        SDL_FreeSurface(surf);
        if (!software) {
            gGraphicsError = std::string("SDL texture creation failed: ") + sdl_error();
            return 0;
        }
        SDL_SetTextureBlendMode(software, SDL_BLENDMODE_BLEND);
        const int64_t id = gNextTexture++;
        gTextures[id] = Texture{0, software, sw, sh};
        return id;
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    int64_t id = gNextTexture++;
    gTextures[id] = Texture{tex, nullptr, surf->w, surf->h};
    SDL_FreeSurface(surf);
    return id;
}
void nexus_gfx_texture_draw(
    int64_t id, double x, double y, double w, double h, double r, double g, double b, double a) {
    auto it = gTextures.find(id);
    if (!gGraphics || it == gTextures.end())
        return;
    if (gSoftware2D) {
        if (!it->second.software)
            return;
        SDL_Rect dst = software_rect(x, y, w, h);
        SDL_RenderCopy(gRenderer, it->second.software, nullptr, &dst);
        return;
    }
    setup_2d();
    draw_textured_quad(
        it->second, float(x), float(y), float(w), float(h), float(r), float(g), float(b), float(a));
}
void nexus_gfx_texture_unload(int64_t id) {
    auto it = gTextures.find(id);
    if (it == gTextures.end())
        return;
    if (it->second.id && glDeleteTextures && gOpenGL3D)
        glDeleteTextures(1, &it->second.id);
    if (it->second.software && SDL_DestroyTexture)
        SDL_DestroyTexture(it->second.software);
    gTextures.erase(it);
}
int64_t nexus_gfx_texture_width(int64_t id) {
    auto it = gTextures.find(id);
    return it == gTextures.end() ? 0 : it->second.w;
}
int64_t nexus_gfx_texture_height(int64_t id) {
    auto it = gTextures.find(id);
    return it == gTextures.end() ? 0 : it->second.h;
}
void nexus_gfx_texture_filter(int64_t id, bool nearest) {
    auto it = gTextures.find(id);
    if (it == gTextures.end() || !gGraphics)
        return;
    if (gSoftware2D) {
        // SDL2 software textures use nearest filtering implicitly.
        (void)nearest;
        return;
    }
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    GLint f = nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void nexus_gfx_camera_set(double x, double y, double zoom) {
    gCameraX = float(x);
    gCameraY = float(y);
    gCameraZoom = zoom > 0.01 ? float(zoom) : 0.01f;
}
void nexus_gfx_camera_reset() {
    gCameraX = 0;
    gCameraY = 0;
    gCameraZoom = 1;
}
void nexus_gfx_texture_draw_frame(int64_t id,
                                  double x,
                                  double y,
                                  double w,
                                  double h,
                                  int64_t frame,
                                  int64_t frameW,
                                  int64_t frameH,
                                  int64_t columns,
                                  double r,
                                  double g,
                                  double b,
                                  double a) {
    auto it = gTextures.find(id);
    if (!gGraphics || it == gTextures.end() || frameW <= 0 || frameH <= 0 || columns <= 0)
        return;
    const int64_t col = frame % columns;
    const int64_t row = frame / columns;
    if (gSoftware2D) {
        if (!it->second.software)
            return;
        SDL_Rect src{int(col * frameW), int(row * frameH), int(frameW), int(frameH)};
        SDL_Rect dst = software_rect(x, y, w, h);
        SDL_RenderCopy(gRenderer, it->second.software, &src, &dst);
        return;
    }
    const float u0 = float(col * frameW) / float(it->second.w);
    const float v0 = float(row * frameH) / float(it->second.h);
    const float u1 = float((col + 1) * frameW) / float(it->second.w);
    const float v1 = float((row + 1) * frameH) / float(it->second.h);
    setup_2d();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    glColor4f(float(r), float(g), float(b), float(a));
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex2f(float(x), float(y));
    glTexCoord2f(u1, v0);
    glVertex2f(float(x + w), float(y));
    glTexCoord2f(u1, v1);
    glVertex2f(float(x + w), float(y + h));
    glTexCoord2f(u0, v1);
    glVertex2f(float(x), float(y + h));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
