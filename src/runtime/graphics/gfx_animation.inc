int64_t nexus_gfx_anim_create(int64_t texture,
                              int64_t frameW,
                              int64_t frameH,
                              int64_t columns,
                              int64_t frameCount,
                              double fps,
                              bool loop) {

    auto it = gTextures.find(texture);

    if (it == gTextures.end() || frameW <= 0 || frameH <= 0 || columns <= 0 || frameCount <= 0 ||
        fps <= 0.0) {
        gGraphicsError = "invalid sprite animation parameters";
        return 0;
    }
    if (frameW > it->second.w || frameH > it->second.h) {
        gGraphicsError = "sprite frame exceeds texture dimensions";
        return 0;
    }
    SpriteAnim a;
    a.texture = texture;
    a.frameW = frameW;
    a.frameH = frameH;
    a.columns = columns;
    a.frameCount = frameCount;
    a.fps = fps;
    a.loop = loop;

    int64_t id = gNextAnim++;
    gAnims[id] = a;
    return id;
}
void nexus_gfx_anim_update(int64_t id, double dt) {
    auto it = gAnims.find(id);
    if (it == gAnims.end())
        return;
    auto& a = it->second;
    if (a.frameCount <= 1)
        return;
    a.time += std::max(dt, 0.0);
    double step = 1.0 / a.fps;
    while (a.time >= step) {
        a.time -= step;
        ++a.frame;
        if (a.frame >= a.frameCount) {
            if (a.loop)
                a.frame = 0;
            else
                a.frame = a.frameCount - 1;
        }
    }
}
void nexus_gfx_anim_draw(
    int64_t id, double x, double y, double w, double h, double r, double g, double b, double a) {
    auto it = gAnims.find(id);
    if (it == gAnims.end())
        return;
    const auto& v = it->second;
    nexus_gfx_texture_draw_frame(
        v.texture, x, y, w, h, v.frame, v.frameW, v.frameH, v.columns, r, g, b, a);
}
int64_t nexus_gfx_anim_frame(int64_t id) {
    auto it = gAnims.find(id);
    return it == gAnims.end() ? -1 : it->second.frame;
}
void nexus_gfx_anim_destroy(int64_t id) {
    gAnims.erase(id);
}

const char* nexus_gfx_image_type() {
    return IMG_Load ? "PNG/JPG/BMP (SDL2_image + SDL2 core)"
                    : "BMP only (install SDL2_image for PNG/JPG)";
}
int64_t nexus_gfx_font_load(const char* path, int64_t size) {
    load_optional_media_libs();
    if (!gTTF || !TTF_Init || !TTF_OpenFont || !TTF_CloseFont || !TTF_RenderUTF8_Blended ||
        size <= 0) {
        gGraphicsError = "TTF font backend unavailable; install SDL2_ttf";
        return 0;
    }
    if (!gTTFReady && TTF_Init() != 0) {
        gGraphicsError = TTF_GetError ? TTF_GetError() : "TTF_Init failed";
        return 0;
    }
    gTTFReady = true;
    TTF_Font* f = TTF_OpenFont(path, int(size));
    if (!f) {
        gGraphicsError = TTF_GetError ? TTF_GetError() : "font load failed";
        return 0;
    }
    int64_t id = gNextFont++;
    gFonts[id] = f;
    return id;
}
void nexus_gfx_text_font(
    int64_t id, double x, double y, const char* text, double r, double g, double b, double a) {
    auto it = gFonts.find(id);
    if (!gGraphics || it == gFonts.end() || !TTF_RenderUTF8_Blended)
        return;
    SDL_Color c{uint8_t(std::clamp(r, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(g, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(b, 0.0, 1.0) * 255.0),
                uint8_t(std::clamp(a, 0.0, 1.0) * 255.0)};
    SDL_Surface* s = TTF_RenderUTF8_Blended(it->second, text ? text : "", c);
    if (!s)
        return;
    if (gSoftware2D) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, s);
        if (texture) {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            SDL_Rect dst{int(std::llround(x)), int(std::llround(y)), s->w, s->h};
            SDL_RenderCopy(gRenderer, texture, nullptr, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(s);
        return;
    }
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(s);
    if (!surf)
        return;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    setup_2d();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(float(x), float(y));
    glTexCoord2f(1, 0);
    glVertex2f(float(x + surf->w), float(y));
    glTexCoord2f(1, 1);
    glVertex2f(float(x + surf->w), float(y + surf->h));
    glTexCoord2f(0, 1);
    glVertex2f(float(x), float(y + surf->h));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &tex);
    SDL_FreeSurface(surf);
}
void nexus_gfx_font_unload(int64_t id) {
    auto it = gFonts.find(id);
    if (it != gFonts.end()) {
        if (TTF_CloseFont)
            TTF_CloseFont(it->second);
        gFonts.erase(it);
    }
}

// Loads a font the user does not have to ship: try well-known system font
// files first and fall back to the runtime's own bundled font if present.
int64_t nexus_gfx_font_default(int64_t size) {
    load_optional_media_libs();
    if (!gTTF || !TTF_OpenFont || size <= 0) {
        gGraphicsError = "TTF font backend unavailable; install SDL2_ttf";
        return 0;
    }
    static const char* candidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
    };
    for (const char* path : candidates) {
        std::FILE* f = std::fopen(path, "rb");
        if (!f)
            continue;
        std::fclose(f);
        int64_t id = nexus_gfx_font_load(path, size);
        if (id != 0)
            return id;
    }
    gGraphicsError = "no system font found; pass an explicit .ttf path to gfx_font_load";
    return 0;
}
int64_t nexus_gfx_text_width(int64_t id, const char* text) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_SizeUTF8)
        return 0;
    int w = 0, h = 0;
    if (TTF_SizeUTF8(it->second, text ? text : "", &w, &h) != 0)
        return 0;
    return w;
}
int64_t nexus_gfx_text_height(int64_t id) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_FontHeight)
        return 0;
    return TTF_FontHeight(it->second);
}
// style: 1 = bold, 2 = italic, 3 = both, 0 = normal
void nexus_gfx_font_style(int64_t id, int64_t style) {
    auto it = gFonts.find(id);
    if (it == gFonts.end() || !TTF_SetFontStyle)
        return;
    int flags = 0;
    if (style & 1)
        flags |= 1; // TTF_STYLE_BOLD
    if (style & 2)
        flags |= 2; // TTF_STYLE_ITALIC
    TTF_SetFontStyle(it->second, flags);
}
int64_t nexus_gfx_sound_load(const char* path);

void nexus_gfx_sound_play(int64_t id, bool loop);

void nexus_gfx_sound_stop();

void nexus_gfx_sound_volume(double v);

int64_t nexus_gfx_music_load(const char* path) {

    load_optional_media_libs();

    if (gMIX && Mix_OpenAudio && Mix_LoadMUS && Mix_FreeMusic && Mix_PlayMusic) {

        if (!gMixerReady && Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 2048) != 0) {
            gGraphicsError = Mix_GetError ? Mix_GetError() : "Mix_OpenAudio failed";
        } else {
            gMixerReady = true;
            Mix_Music* m = Mix_LoadMUS(path);
            if (m) {
                if (gMusic && Mix_FreeMusic)
                    Mix_FreeMusic(gMusic);
                gMusic = m;
                return 1;
            }
            gGraphicsError = Mix_GetError ? Mix_GetError() : "music load failed";
        }
    }
    // Guaranteed fallback: plain PCM WAV using the core SDL2 audio queue.
    return nexus_gfx_sound_load(path);
}
bool nexus_gfx_music_play(int64_t id, bool loop) {

    if (gMusic && Mix_PlayMusic)
        return Mix_PlayMusic(gMusic, loop ? -1 : 1) == 0;

    nexus_gfx_sound_play(id, loop);
    return gAudioDevice != 0;
}
void nexus_gfx_music_stop() {
    if (Mix_HaltMusic && gMusic)
        Mix_HaltMusic();
    nexus_gfx_sound_stop();
}
void nexus_gfx_music_volume(double v) {
    if (Mix_VolumeMusic && gMusic)
        Mix_VolumeMusic(std::clamp(int(v * 128.0), 0, 128));
    nexus_gfx_sound_volume(v);
}
void nexus_gfx_cursor_visible(bool visible) {
    const int requestedState = visible ? 1 : 0;
    if (SDL_ShowCursor && gCursorVisibleState != requestedState) {
        SDL_ShowCursor(requestedState);
        gCursorVisibleState = requestedState;
    }
}
