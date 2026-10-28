
    static void apply_software_layout() {
        if (!gRenderer)
            return;
        const int logicalW = gUiWidth > 0.0 ? int(std::llround(gUiWidth)) : gWidth;
        const int logicalH = gUiHeight > 0.0 ? int(std::llround(gUiHeight)) : gHeight;
        SDL_RenderSetLogicalSize(gRenderer, logicalW, logicalH);
        SDL_RenderSetViewport(gRenderer, nullptr);
        // SDL's software renderer defaults to SDL_BLENDMODE_NONE, which silently
        // ignores the alpha channel of the draw colour. Enable blending so
        // translucent rectangles, overlays, circles and bitmap text compose
        // correctly instead of painting as opaque blocks.
        if (SDL_SetRenderDrawBlendMode)
            SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
    }

    static void software_color(double r, double g, double b, double a) {
        if (!gRenderer)
            return;
        SDL_SetRenderDrawColor(gRenderer,
                               uint8_t(std::clamp(r, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(g, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(b, 0.0, 1.0) * 255.0),
                               uint8_t(std::clamp(a, 0.0, 1.0) * 255.0));
    }

    static SDL_Rect software_rect(double x, double y, double w, double h) {
        return SDL_Rect{int(std::llround(x)),
                        int(std::llround(y)),
                        std::max(0, int(std::llround(w))),
                        std::max(0, int(std::llround(h)))};
    }

    static void software_fill_rect(double x, double y, double w, double h) {
        if (!gRenderer)
            return;
        SDL_Rect rect = software_rect(x, y, w, h);
        SDL_RenderFillRect(gRenderer, &rect);
    }

    static void update_window_metrics() {

        if (!gWindow || !SDL_GetWindowSize) {

            return;
        }

        int width = 0;

        int height = 0;

        SDL_GetWindowSize(gWindow, &width, &height);

        if (width > 0) {

            gWidth = width;
        }
        if (height > 0) {

            gHeight = height;
        }

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        gViewportX = 0;

        gViewportY = 0;

        gViewportWidth = gWidth;

        gViewportHeight = gHeight;

        if (gUiWidth <= 0.0 || gUiHeight <= 0.0 || gWidth <= 0 || gHeight <= 0) {

            return;
        }

        const double scaleX = static_cast<double>(gWidth) / logicalWidth;

        const double scaleY = static_cast<double>(gHeight) / logicalHeight;

        const double scale = std::min(scaleX, scaleY);

        gViewportWidth = std::max(1, static_cast<int>(std::llround(logicalWidth * scale)));

        gViewportHeight = std::max(1, static_cast<int>(std::llround(logicalHeight * scale)));

        gViewportX = (gWidth - gViewportWidth) / 2;

        gViewportY = (gHeight - gViewportHeight) / 2;
    }

    static void apply_2d_viewport() {

        update_window_metrics();

        glViewport(gViewportX, gViewportY, gViewportWidth, gViewportHeight);
    }

    static void window_to_ui(double windowX, double windowY, double& uiX, double& uiY) {

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        if (gUiWidth <= 0.0 || gUiHeight <= 0.0 || gViewportWidth <= 0 || gViewportHeight <= 0) {

            uiX = windowX;

            uiY = windowY;

            return;
        }

        uiX = (windowX - static_cast<double>(gViewportX)) * logicalWidth /
              static_cast<double>(gViewportWidth);

        uiY = (windowY - static_cast<double>(gViewportY)) * logicalHeight /
              static_cast<double>(gViewportHeight);
    }
