
// UTF-8 byte-count-limited immediate-mode text input. The field has pointer
// focus, handles text events and Backspace by Unicode code point, and finishes
// editing on Enter or Tab. Selection/caret movement and IME editing commands
// are outside this minimal runtime widget.
char* nexus_ui_text_field(double x, double y, double w, double h,
                          const char* current, const char* placeholder, int64_t maxBytes) {
    std::string value = current ? current : "";
    bool valueChanged = false;

    // The input string is usually the previous frame's result. Borrow it when
    // the text stays the same; allocate only when truncation or user input
    // produces a genuinely new value. This avoids a per-frame allocation/leak.
    const auto resultValue = [&]() -> char* {
        if (!valueChanged && current)
            return const_cast<char*>(current);
        return nx_strdup(value.c_str());
    };
    auto safePrefix = [](const std::string& text, size_t limit) -> size_t {
        size_t end = std::min(limit, text.size());
        // If the cutoff is inside a UTF-8 code point, discard that whole code point.
        while (end > 0 && end < text.size() &&
               (static_cast<uint8_t>(text[end]) & 0xC0u) == 0x80u)
            --end;
        return end;
    };
    auto popCodePoint = [](std::string& text) {
        if (text.empty())
            return;
        size_t start = text.size() - 1;
        while (start > 0 &&
               (static_cast<uint8_t>(text[start]) & 0xC0u) == 0x80u)
            --start;
        text.resize(start);
    };

    const bool editable = maxBytes > 0;
    const size_t limit = editable
        ? static_cast<size_t>(std::clamp<int64_t>(maxBytes, 1, 8192))
        : std::max<size_t>(value.size(), 1);
    if (value.size() > limit) {
        value.resize(safePrefix(value, limit));
        valueChanged = true;
    }

    if (w <= 0.0 || h <= 0.0 || !gGraphics)
        return resultValue();

    const double mx = nexus_gfx_mouse_x();
    const double my = nexus_gfx_mouse_y();
    const bool hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    const bool sameField = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;

    if (nexus_gfx_mouse_clicked(1)) {
        if (hover && editable) {
            if (!sameField && SDL_StartTextInput)
                SDL_StartTextInput();
            gUiTextFieldFocused = true;
            gUiTextFieldX = x;
            gUiTextFieldY = y;
            gUiTextFieldWidth = w;
            gUiTextFieldHeight = h;
        } else if (sameField) {
            gUiTextFieldFocused = false;
            if (SDL_StopTextInput)
                SDL_StopTextInput();
            gTextInputThisFrame.clear();
        }
    }

    const bool focused = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;

    if (focused && editable) {
        // SDL_SCANCODE_BACKSPACE == 42.
        if (gKeyPressed.size() > 42 && gKeyPressed[42]) {
            const size_t previousSize = value.size();
            popCodePoint(value);
            valueChanged = valueChanged || value.size() != previousSize;
        }

        if (!gTextInputThisFrame.empty() && value.size() < limit) {
            const size_t allowed = limit - value.size();
            const size_t addBytes = safePrefix(gTextInputThisFrame, allowed);
            if (addBytes > 0) {
                value.append(gTextInputThisFrame.data(), addBytes);
                valueChanged = true;
            }
        }
        gTextInputThisFrame.clear();

        // Enter finishes editing; Tab releases this field for application navigation.
        if ((gKeyPressed.size() > 40 && gKeyPressed[40]) ||
            (gKeyPressed.size() > 43 && gKeyPressed[43])) {
            gUiTextFieldFocused = false;
            if (SDL_StopTextInput)
                SDL_StopTextInput();
        }
    }

    const bool currentlyFocused = gUiTextFieldFocused &&
        gUiTextFieldX == x && gUiTextFieldY == y &&
        gUiTextFieldWidth == w && gUiTextFieldHeight == h;
    const double bg = editable ? 0.035 : 0.025;
    nexus_gfx_rect(x, y, w, h, bg, bg + 0.018, bg + 0.035, 1.0);
    const double borderR = currentlyFocused ? 0.20 : (hover ? 0.20 : 0.12);
    const double borderG = currentlyFocused ? 0.72 : (hover ? 0.42 : 0.18);
    const double borderB = currentlyFocused ? 0.96 : (hover ? 0.58 : 0.28);
    const double thickness = currentlyFocused ? 2.0 : 1.0;
    nexus_gfx_rect(x, y, w, thickness, borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x, y + h - thickness, w, thickness, borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x, y + thickness, thickness, std::max(0.0, h - 2.0 * thickness),
                    borderR, borderG, borderB, 1.0);
    nexus_gfx_rect(x + w - thickness, y + thickness, thickness,
                    std::max(0.0, h - 2.0 * thickness), borderR, borderG, borderB, 1.0);

    const size_t visibleBudget = w > 28.0 ? static_cast<size_t>((w - 24.0) / 9.0) : 0u;
    std::string visible = value;
    if (visible.size() > visibleBudget) {
        size_t start = visible.size() - visibleBudget;
        while (start < visible.size() &&
               (static_cast<uint8_t>(visible[start]) & 0xC0u) == 0x80u)
            ++start;
        visible = visible.substr(start);
    }

    const bool empty = value.empty();
    const char* displayed = empty ? (placeholder ? placeholder : "") : visible.c_str();
    const double textY = y + std::max(3.0, (h - 19.0) * 0.5);
    if (empty)
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.43, 0.51, 0.62, 1.0);
    else
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.91, 0.95, 1.0, 1.0);

    if (currentlyFocused && editable && std::fmod(now_seconds(), 1.0) < 0.55) {
        const double cursorOffset = std::min(std::max(0.0, w - 22.0),
                                             static_cast<double>(visible.size()) * 9.0);
        nexus_gfx_rect(x + 10.0 + cursorOffset, y + 8.0, 1.5,
                       std::max(2.0, h - 16.0), 0.35, 0.84, 1.0, 1.0);
    }

    return resultValue();
}

bool nexus_ui_button(double x, double y, double w, double h, const char* label) {
    if (w <= 0.0 || h <= 0.0)
        return false;
    bool hover = false;
    if (gGraphics) {
        const double mx = nexus_gfx_mouse_x();
        const double my = nexus_gfx_mouse_y();
        hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
    nexus_gfx_rect(x, y, w, h, hover ? 0.12 : 0.08, hover ? 0.48 : 0.32, 0.82, 1.0);
    nexus_gfx_text(x + 12, y + 12, 1.5, label ? label : "", 1, 1, 1, 1);
    // Buttons activate only on a left-button press edge, not right/middle clicks
    // and not on every frame while the mouse button remains held.
    return hover && nexus_gfx_mouse_clicked(1);
}
void nexus_ui_panel(
    double x, double y, double w, double h, double r, double g, double b, double a) {
    nexus_gfx_rect(x, y, w, h, r, g, b, a);
}
void nexus_ui_label(double x, double y, const char* text, double scale) {
    nexus_gfx_text(x, y, scale, text, 1, 1, 1, 1);
}
void nexus_ui_progress(
    double x, double y, double w, double h, double value, double r, double g, double b, double a) {
    nexus_gfx_rect(x, y, w, h, 0.12, 0.14, 0.18, 1);
    double v = std::clamp(value, 0.0, 1.0);
    nexus_gfx_rect(x, y, w * v, h, r, g, b, a);
}
bool nexus_ui_checkbox(double x, double y, const char* label, bool checked) {
    bool hover = false;
    if (gGraphics) {
        const double mx = nexus_gfx_mouse_x();
        const double my = nexus_gfx_mouse_y();
        // The caption is part of the hit target, so users do not need to hit
        // the small square precisely.
        const double labelWidth = label ? static_cast<double>(std::strlen(label)) * 10.0 : 0.0;
        hover = mx >= x && mx <= x + 40.0 + labelWidth && my >= y && my <= y + 28.0;
    }
    bool on = checked;
    if (hover && nexus_gfx_mouse_clicked(1))
        on = !checked;
    nexus_gfx_rect(x, y, 28, 28, 0.10, 0.12, 0.18, 1.0);
    if (on)
        nexus_gfx_rect(x + 5, y + 5, 18, 18, 0.2, 0.75, 1.0, 1.0);
    if (label)
        nexus_gfx_text(x + 40, y + 2, 1.4, label, 1, 1, 1, 1);
    return on;
}
double nexus_ui_slider(double x, double y, double w, double h, double value) {
    // std::clamp leaves NaN unchanged, which would propagate into drawing math.
    double v = std::clamp(std::isnan(value) ? 0.0 : value, 0.0, 1.0);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) ||
        !std::isfinite(h) || !std::isfinite(x + w) || !std::isfinite(y + h) ||
        w <= 0.0 || h <= 0.0)
        return v;

    double mx = 0.0;
    double my = 0.0;
    bool down = false;
    if (gGraphics) {
        mx = nexus_gfx_mouse_x();
        my = nexus_gfx_mouse_y();
        down = nexus_gfx_mouse_down(1);
    }
    const bool hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    if (!down)
        gUiSliderDragging = false;
    if (nexus_gfx_mouse_clicked(1) && hover) {
        gUiSliderDragging = true;
        gUiSliderX = x;
        gUiSliderY = y;
        gUiSliderWidth = w;
        gUiSliderHeight = h;
    }

    // Capture the slider from mouse-down until mouse-up. Once dragging starts,
    // keep updating it even if the cursor leaves the original track.
    const bool isActiveSlider = gUiSliderDragging &&
        gUiSliderX == x && gUiSliderY == y &&
        gUiSliderWidth == w && gUiSliderHeight == h;
    if (down && isActiveSlider)
        v = std::clamp((mx - x) / w, 0.0, 1.0);

    nexus_gfx_rect(x, y + h * 0.35, w, h * 0.3, 0.12, 0.14, 0.18, 1.0);
    nexus_gfx_rect(x, y + h * 0.35, w * v, h * 0.3, 0.2, 0.65, 1.0, 1.0);
    nexus_gfx_circle(x + w * v, y + h * 0.5, h * 0.7, 0.9, 0.95, 1.0, 1.0);
    return v;
}
