// Immediate-mode UTF-8 text fields. The legacy API uses rectangle identity;
// ui_text_field_id accepts a stable positive widget ID. Editing state is shared
// by the focused field and the caller retains the returned string.
static bool nexus_ui_utf8_continuation(uint8_t byte) {
    return (byte & 0xC0u) == 0x80u;
}

static size_t nexus_ui_utf8_safe_prefix(const std::string& text, size_t limit) {
    size_t end = std::min(limit, text.size());
    while (end > 0 && end < text.size() &&
           nexus_ui_utf8_continuation(static_cast<uint8_t>(text[end]))) {
        --end;
    }
    return end;
}

static size_t nexus_ui_utf8_previous(const std::string& text, size_t position) {
    position = std::min(position, text.size());
    if (position == 0)
        return 0;
    --position;
    while (position > 0 &&
           nexus_ui_utf8_continuation(static_cast<uint8_t>(text[position]))) {
        --position;
    }
    return position;
}

static size_t nexus_ui_utf8_next(const std::string& text, size_t position) {
    position = std::min(position, text.size());
    if (position >= text.size())
        return text.size();
    ++position;
    while (position < text.size() &&
           nexus_ui_utf8_continuation(static_cast<uint8_t>(text[position]))) {
        ++position;
    }
    return position;
}

static size_t nexus_ui_utf8_column(const std::string& text, size_t byteOffset) {
    byteOffset = std::min(byteOffset, text.size());
    size_t column = 0;
    for (size_t at = 0; at < byteOffset; at = nexus_ui_utf8_next(text, at))
        ++column;
    return column;
}

static size_t nexus_ui_utf8_offset(const std::string& text, size_t column) {
    size_t at = 0;
    while (column > 0 && at < text.size()) {
        at = nexus_ui_utf8_next(text, at);
        --column;
    }
    return at;
}

static int64_t nexus_ui_text_field_identity(int64_t widgetId,
                                            double x,
                                            double y,
                                            double w,
                                            double h) {
    if (widgetId > 0)
        return widgetId;

    // Preserve the legacy rectangle-based identity while giving every field a
    // non-zero key that can participate in ordered Tab traversal.
    uint64_t hash = UINT64_C(1469598103934665603);
    const auto mix = [&](const void* data, size_t length) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < length; ++i) {
            hash ^= bytes[i];
            hash *= UINT64_C(1099511628211);
        }
    };
    mix(&x, sizeof(x));
    mix(&y, sizeof(y));
    mix(&w, sizeof(w));
    mix(&h, sizeof(h));
    hash |= UINT64_C(1) << 63;
    return static_cast<int64_t>(hash);
}

static bool nexus_ui_text_field_is_focused(int64_t identity,
                                           double x,
                                           double y,
                                           double w,
                                           double h) {
    if (!gUiTextFieldFocused || gUiTextFieldFocusId != identity)
        return false;
    // Keep the rectangle check for legacy controls and to make accidental
    // duplicate IDs fail closed rather than sharing one caret.
    if (identity < 0) {
        return gUiTextFieldX == x && gUiTextFieldY == y &&
               gUiTextFieldWidth == w && gUiTextFieldHeight == h;
    }
    return true;
}

static void nexus_ui_text_field_focus(int64_t identity,
                                      double x,
                                      double y,
                                      double w,
                                      double h,
                                      const std::string& value) {
    if (!gUiTextFieldFocused && SDL_StartTextInput)
        SDL_StartTextInput();
    gUiTextFieldFocused = true;
    gUiTextFieldFocusId = identity;
    gUiTextFieldX = x;
    gUiTextFieldY = y;
    gUiTextFieldWidth = w;
    gUiTextFieldHeight = h;
    gUiTextFieldOriginal = value;
    gUiTextFieldCaret = value.size();
    gUiTextFieldAnchor = value.size();
}

static void nexus_ui_text_field_blur(bool preserveTraversal = false) {
    gUiTextFieldFocused = false;
    gUiTextFieldFocusId = 0;
    gUiTextFieldCaret = 0;
    gUiTextFieldAnchor = 0;
    if (!preserveTraversal)
        gUiTextFieldReverseTarget = 0;
    if (SDL_StopTextInput)
        SDL_StopTextInput();
}

static bool nexus_ui_text_delete_selection(std::string& text,
                                           size_t& caret,
                                           size_t& anchor) {
    caret = std::min(caret, text.size());
    anchor = std::min(anchor, text.size());
    const size_t begin = std::min(caret, anchor);
    const size_t end = std::max(caret, anchor);
    if (begin == end)
        return false;
    text.erase(begin, end - begin);
    caret = begin;
    anchor = begin;
    return true;
}

static bool nexus_ui_text_insert(std::string& text,
                                 size_t& caret,
                                 size_t& anchor,
                                 std::string_view inserted,
                                 size_t limit) {
    caret = std::min(caret, text.size());
    anchor = std::min(anchor, text.size());
    const size_t begin = std::min(caret, anchor);
    const size_t end = std::max(caret, anchor);
    std::string replacement = text;
    replacement.erase(begin, end - begin);
    const size_t available = replacement.size() < limit ? limit - replacement.size() : 0;
    const std::string input(inserted);
    const size_t bytes = nexus_ui_utf8_safe_prefix(input, available);
    if (!input.empty() && bytes == 0)
        return false;
    if (bytes > 0)
        replacement.insert(begin, input.data(), bytes);
    const bool changed = replacement != text;
    text.swap(replacement);
    caret = begin + bytes;
    anchor = caret;
    return changed;
}

static bool nexus_ui_text_backspace(std::string& text, size_t& caret, size_t& anchor) {
    if (nexus_ui_text_delete_selection(text, caret, anchor))
        return true;
    caret = std::min(caret, text.size());
    if (caret == 0)
        return false;
    const size_t previous = nexus_ui_utf8_previous(text, caret);
    text.erase(previous, caret - previous);
    caret = previous;
    anchor = caret;
    return true;
}

static bool nexus_ui_text_delete_forward(std::string& text, size_t& caret, size_t& anchor) {
    if (nexus_ui_text_delete_selection(text, caret, anchor))
        return true;
    caret = std::min(caret, text.size());
    if (caret >= text.size())
        return false;
    const size_t next = nexus_ui_utf8_next(text, caret);
    text.erase(caret, next - caret);
    anchor = caret;
    return true;
}

static char* nexus_ui_text_field_impl(double x,
                                      double y,
                                      double w,
                                      double h,
                                      const char* current,
                                      const char* placeholder,
                                      int64_t maxBytes,
                                      int64_t widgetId) {
    std::string value = current ? current : "";
    bool valueChanged = false;
    const auto resultValue = [&]() -> char* {
        if (!valueChanged && current)
            return const_cast<char*>(current);
        return nx_strdup(value.c_str());
    };

    const bool editable = maxBytes > 0;
    const size_t limit = editable
        ? static_cast<size_t>(std::clamp<int64_t>(maxBytes, 1, 8192))
        : std::max<size_t>(value.size(), 1);
    if (value.size() > limit) {
        value.resize(nexus_ui_utf8_safe_prefix(value, limit));
        valueChanged = true;
    }
    if (w <= 0.0 || h <= 0.0 || !gGraphics)
        return resultValue();

    if (nexus_gfx_mouse_clicked(1)) {
        // A direct pointer action supersedes a pending keyboard traversal.
        gUiTextFieldAdvancePending = false;
        gUiTextFieldReverseTarget = 0;
    }
    const int64_t identity = nexus_ui_text_field_identity(widgetId, x, y, w, h);
    if (editable) {
        // A forward traversal is consumed by the next editable field in source
        // order. If the current field was last, it remains pending and wraps to
        // the first field on the next frame.
        if (gUiTextFieldAdvancePending && identity != gUiTextFieldFocusId) {
            gUiTextFieldAdvancePending = false;
            gUiTextFieldReverseTarget = 0;
            nexus_ui_text_field_focus(identity, x, y, w, h, value);
        } else if (gUiTextFieldReverseTarget == identity) {
            gUiTextFieldReverseTarget = 0;
            gUiTextFieldAdvancePending = false;
            nexus_ui_text_field_focus(identity, x, y, w, h, value);
        }
    } else if (nexus_ui_text_field_is_focused(identity, x, y, w, h)) {
        nexus_ui_text_field_blur();
    }

    const double mx = nexus_gfx_mouse_x();
    const double my = nexus_gfx_mouse_y();
    const bool hover = mx >= x && mx <= x + w && my >= y && my <= y + h;
    bool focused = nexus_ui_text_field_is_focused(identity, x, y, w, h);

    if (nexus_gfx_mouse_clicked(1)) {
        if (hover && editable) {
            if (!focused) {
                nexus_ui_text_field_focus(identity, x, y, w, h, value);
                focused = true;
            }
            const size_t visibleBudget = w > 28.0
                ? static_cast<size_t>((w - 24.0) / 9.0) : 0u;
            const size_t totalColumns = nexus_ui_utf8_column(value, value.size());
            const size_t cursorColumn = std::min(nexus_ui_utf8_column(value, gUiTextFieldCaret),
                                                 totalColumns);
            size_t visibleStart = totalColumns > visibleBudget
                ? totalColumns - visibleBudget : 0u;
            if (cursorColumn < visibleStart)
                visibleStart = cursorColumn;
            if (visibleBudget > 0 && cursorColumn >= visibleStart + visibleBudget)
                visibleStart = cursorColumn - visibleBudget + 1u;
            const double localX = std::max(0.0, mx - (x + 10.0));
            const size_t clickedColumn = static_cast<size_t>(localX / 9.0);
            const size_t targetColumn =
                std::min(totalColumns, visibleStart + clickedColumn);
            gUiTextFieldCaret = nexus_ui_utf8_offset(value, targetColumn);
            gUiTextFieldAnchor = gUiTextFieldCaret;
        } else if (focused) {
            nexus_ui_text_field_blur();
            focused = false;
            gTextInputThisFrame.clear();
        }
    }

    focused = nexus_ui_text_field_is_focused(identity, x, y, w, h);
    if (focused && editable) {
        gUiTextFieldCaret = std::min(gUiTextFieldCaret, value.size());
        gUiTextFieldAnchor = std::min(gUiTextFieldAnchor, value.size());
        gUiTextFieldCaret = nexus_ui_utf8_offset(
            value, nexus_ui_utf8_column(value, gUiTextFieldCaret));
        gUiTextFieldAnchor = nexus_ui_utf8_offset(
            value, nexus_ui_utf8_column(value, gUiTextFieldAnchor));

        const auto pressed = [](size_t scanCode) {
            return gKeyPressed.size() > scanCode && gKeyPressed[scanCode] != 0;
        };
        const bool shiftDown =
            (gKeys.size() > 225 && gKeys[225]) || (gKeys.size() > 229 && gKeys[229]);
        const bool controlDown =
            (gKeys.size() > 224 && gKeys[224]) || (gKeys.size() > 228 && gKeys[228]);

        if (pressed(41)) {
            if (value != gUiTextFieldOriginal) {
                value = gUiTextFieldOriginal;
                valueChanged = true;
            }
            nexus_ui_text_field_blur();
            focused = false;
            gTextInputThisFrame.clear();
        } else if (pressed(43)) {
            const int64_t previous = gUiTextFieldPreviousInFrame;
            const int64_t previousFrameLast = gUiTextFieldLastPreviousFrame;
            const int64_t target = shiftDown
                ? (previous != 0 ? previous : previousFrameLast) : 0;
            if (shiftDown && target != 0) {
                gUiTextFieldReverseTarget = target;
                gUiTextFieldAdvancePending = false;
            } else {
                gUiTextFieldAdvancePending = true;
                gUiTextFieldReverseTarget = 0;
            }
            nexus_ui_text_field_blur(true);
            focused = false;
            gTextInputThisFrame.clear();
        } else if (pressed(40)) {
            nexus_ui_text_field_blur();
            focused = false;
            gTextInputThisFrame.clear();
        } else {
            if (controlDown && pressed(4)) {
                gUiTextFieldAnchor = 0;
                gUiTextFieldCaret = value.size();
            }
            if (pressed(80)) {
                if (!shiftDown && gUiTextFieldCaret != gUiTextFieldAnchor) {
                    gUiTextFieldCaret = std::min(gUiTextFieldCaret, gUiTextFieldAnchor);
                } else {
                    gUiTextFieldCaret = nexus_ui_utf8_previous(value, gUiTextFieldCaret);
                }
                if (!shiftDown)
                    gUiTextFieldAnchor = gUiTextFieldCaret;
            }
            if (pressed(79)) {
                if (!shiftDown && gUiTextFieldCaret != gUiTextFieldAnchor) {
                    gUiTextFieldCaret = std::max(gUiTextFieldCaret, gUiTextFieldAnchor);
                } else {
                    gUiTextFieldCaret = nexus_ui_utf8_next(value, gUiTextFieldCaret);
                }
                if (!shiftDown)
                    gUiTextFieldAnchor = gUiTextFieldCaret;
            }
            if (pressed(74)) {
                gUiTextFieldCaret = 0;
                if (!shiftDown)
                    gUiTextFieldAnchor = gUiTextFieldCaret;
            }
            if (pressed(77)) {
                gUiTextFieldCaret = value.size();
                if (!shiftDown)
                    gUiTextFieldAnchor = gUiTextFieldCaret;
            }
            if (pressed(42) && nexus_ui_text_backspace(
                    value, gUiTextFieldCaret, gUiTextFieldAnchor)) {
                valueChanged = true;
            }
            if (pressed(76) && nexus_ui_text_delete_forward(
                    value, gUiTextFieldCaret, gUiTextFieldAnchor)) {
                valueChanged = true;
            }
            if (!gTextInputThisFrame.empty()) {
                if (nexus_ui_text_insert(value,
                                         gUiTextFieldCaret,
                                         gUiTextFieldAnchor,
                                         gTextInputThisFrame,
                                         limit)) {
                    valueChanged = true;
                }
            }
            gTextInputThisFrame.clear();
        }
    }

    const bool currentlyFocused = nexus_ui_text_field_is_focused(identity, x, y, w, h);
    if (editable) {
        gUiTextFieldPreviousInFrame = identity;
        gUiTextFieldLastInFrame = identity;
    }

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

    const size_t totalColumns = nexus_ui_utf8_column(value, value.size());
    const size_t caretColumn = nexus_ui_utf8_column(value, gUiTextFieldCaret);
    const size_t anchorColumn = nexus_ui_utf8_column(value, gUiTextFieldAnchor);
    const size_t visibleBudget = w > 28.0
        ? static_cast<size_t>((w - 24.0) / 9.0) : 0u;
    size_t visibleStart = totalColumns > visibleBudget
        ? totalColumns - visibleBudget : 0u;
    if (currentlyFocused && caretColumn < visibleStart)
        visibleStart = caretColumn;
    if (currentlyFocused && visibleBudget > 0 && caretColumn >= visibleStart + visibleBudget)
        visibleStart = caretColumn - visibleBudget + 1u;
    const size_t visibleEnd = std::min(totalColumns, visibleStart + visibleBudget);
    const size_t visibleByteStart = nexus_ui_utf8_offset(value, visibleStart);
    const size_t visibleByteEnd = nexus_ui_utf8_offset(value, visibleEnd);
    const std::string visible = value.substr(visibleByteStart, visibleByteEnd - visibleByteStart);

    if (currentlyFocused && editable && gUiTextFieldCaret != gUiTextFieldAnchor) {
        const size_t selectionStart = std::min(caretColumn, anchorColumn);
        const size_t selectionEnd = std::max(caretColumn, anchorColumn);
        const size_t clippedStart = std::max(selectionStart, visibleStart);
        const size_t clippedEnd = std::min(selectionEnd, visibleEnd);
        if (clippedEnd > clippedStart) {
            const double selectionX = x + 10.0 +
                static_cast<double>(clippedStart - visibleStart) * 9.0;
            const double selectionWidth =
                static_cast<double>(clippedEnd - clippedStart) * 9.0;
            nexus_gfx_rect(selectionX, y + 7.0, selectionWidth,
                           std::max(2.0, h - 14.0), 0.13, 0.38, 0.62, 0.9);
        }
    }

    const bool empty = value.empty();
    const char* displayed = empty ? (placeholder ? placeholder : "") : visible.c_str();
    const double textY = y + std::max(3.0, (h - 19.0) * 0.5);
    if (empty)
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.43, 0.51, 0.62, 1.0);
    else
        nexus_gfx_text(x + 10.0, textY, 1.2, displayed, 0.91, 0.95, 1.0, 1.0);

    if (currentlyFocused && editable && std::fmod(now_seconds(), 1.0) < 0.55) {
        const size_t caretVisibleColumn =
            caretColumn > visibleStart ? caretColumn - visibleStart : 0u;
        const double cursorOffset = std::min(
            std::max(0.0, w - 22.0), static_cast<double>(caretVisibleColumn) * 9.0);
        nexus_gfx_rect(x + 10.0 + cursorOffset, y + 8.0, 1.5,
                       std::max(2.0, h - 16.0), 0.35, 0.84, 1.0, 1.0);
    }

    return resultValue();
}

char* nexus_ui_text_field_id(double x,
                             double y,
                             double w,
                             double h,
                             const char* current,
                             const char* placeholder,
                             int64_t maxBytes,
                             int64_t widgetId) {
    return nexus_ui_text_field_impl(
        x, y, w, h, current, placeholder, maxBytes, widgetId);
}

char* nexus_ui_text_field(double x,
                          double y,
                          double w,
                          double h,
                          const char* current,
                          const char* placeholder,
                          int64_t maxBytes) {
    return nexus_ui_text_field_impl(
        x, y, w, h, current, placeholder, maxBytes, 0);
}

#ifdef NEXUS_RUNTIME_TESTING
extern "C" int nexus_runtime_test_ui_text_edit(char* buffer,
                                                size_t capacity,
                                                int64_t* caret,
                                                int64_t* anchor,
                                                const char* inserted,
                                                int operation,
                                                int64_t maxBytes) {
    if (!buffer || capacity == 0 || !caret || !anchor || maxBytes < 1)
        return -1;
    std::string value(buffer);
    size_t cursor = *caret < 0 ? 0u : std::min(static_cast<size_t>(*caret), value.size());
    size_t selection = *anchor < 0 ? 0u : std::min(static_cast<size_t>(*anchor), value.size());
    cursor = nexus_ui_utf8_offset(value, nexus_ui_utf8_column(value, cursor));
    selection = nexus_ui_utf8_offset(value, nexus_ui_utf8_column(value, selection));
    const size_t limit = static_cast<size_t>(std::clamp<int64_t>(maxBytes, 1, 8192));
    bool changed = false;
    if (operation == 0) {
        changed = nexus_ui_text_insert(
            value, cursor, selection, inserted ? std::string_view(inserted) : std::string_view{}, limit);
    } else if (operation == 1) {
        changed = nexus_ui_text_backspace(value, cursor, selection);
    } else if (operation == 2) {
        changed = nexus_ui_text_delete_forward(value, cursor, selection);
    } else if (operation == 3) {
        cursor = nexus_ui_utf8_previous(value, cursor);
        selection = cursor;
    } else if (operation == 4) {
        cursor = nexus_ui_utf8_next(value, cursor);
        selection = cursor;
    } else {
        return -1;
    }
    if (value.size() + 1 > capacity)
        return -1;
    std::memcpy(buffer, value.c_str(), value.size() + 1);
    *caret = static_cast<int64_t>(cursor);
    *anchor = static_cast<int64_t>(selection);
    return changed ? 1 : 0;
}
#endif

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
