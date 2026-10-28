    static int64_t scancode_for(const char* name) {

        if (!name)
            return -1;
        std::string s(name);
        std::transform(
            s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::toupper(c)); });

        if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z') {
            static const int codes[26] = {4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                                          17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};
            return codes[s[0] - 'A'];
        }
        if (s.size() == 1 && s[0] >= '0' && s[0] <= '9') {
            static const int codes[10] = {39, 30, 31, 32, 33, 34, 35, 36, 37, 38};
            return codes[s[0] - '0'];
        }
        if (s == "SPACE")
            return 44;
        if (s == "ENTER")
            return 40;
        if (s == "ESC" || s == "ESCAPE")
            return 41;
        if (s == "LEFT")
            return 80;
        if (s == "RIGHT")
            return 79;
        if (s == "UP")
            return 82;
        if (s == "DOWN")
            return 81;
        if (s == "TAB")
            return 43;
        if (s.size() >= 2 && s[0] == 'F' && s[1] >= '1' && s[1] <= '9') {
            // SDL scancodes for F1..F10 are 58..67; F11/F12 are 68/69.
            int n = 0;
            for (size_t i = 1; i < s.size(); ++i) {
                if (s[i] < '0' || s[i] > '9')
                    return -1;
                n = n * 10 + (s[i] - '0');
            }
            if (n >= 1 && n <= 10)
                return 58 + (n - 1);
            if (n == 11)
                return 68;
            if (n == 12)
                return 69;
            return -1;
        }
        if (s == "SHIFT")
            return 225;
        if (s == "CTRL" || s == "CONTROL")
            return 224;
        return -1;
    }

    static void clear_event() {
        gLastEvent = 0;
        gLastKey = -1;
        gLastMouseButton = 0;
        std::fill(gKeyPressed.begin(), gKeyPressed.end(), uint8_t(0));
        gMouseClickedButtons.fill(0);
        gTextInputThisFrame.clear();
    }
    static void process_events() {

        if (!SDL_PollEvent)
            return;
        SDL_Event e{};

        clear_event();

        while (SDL_PollEvent(&e)) {

            gLastEvent = int(e.type);

            if (e.type == SDL_QUIT) {
                gClose = true;
            } else if (e.type == SDL_TEXTINPUT) {
                size_t textLength = 0;
                while (textLength < sizeof(e.text.text) && e.text.text[textLength] != '\0')
                    ++textLength;
                gTextInputThisFrame.append(e.text.text, textLength);
            } else if (e.type == SDL_KEYDOWN) {
                gLastKey = int(e.key.keysym.scancode);
                if (gLastKey >= 0 && gLastKey < int(gKeys.size())) {
                    gKeys[size_t(gLastKey)] = 1;
                    if (e.key.repeat == 0)
                        gKeyPressed[size_t(gLastKey)] = 1;
                }
                // Escape closes the window by default (SDL_SCANCODE_ESCAPE == 41).
                if (gLastKey == 41)
                    gClose = true;
            } else if (e.type == SDL_KEYUP) {
                gLastKey = int(e.key.keysym.scancode);
                if (gLastKey >= 0 && gLastKey < int(gKeys.size()))
                    gKeys[size_t(gLastKey)] = 0;
            } else if (e.type == SDL_MOUSEMOTION) {
                gLastX = e.motion.x;
                gLastY = e.motion.y;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                gLastMouseButton = int(e.button.button);
                gLastX = e.button.x;
                gLastY = e.button.y;
                if (e.button.button < gMouseButtons.size())
                    gMouseButtons[e.button.button] = 1;
                if (e.button.button < gMouseClickedButtons.size())
                    gMouseClickedButtons[e.button.button] = 1;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == 1)
                    gUiSliderDragging = false;
                gLastMouseButton = int(e.button.button);
                gLastX = e.button.x;
                gLastY = e.button.y;
                if (e.button.button < gMouseButtons.size())
                    gMouseButtons[e.button.button] = 0;
            } else if (e.type == SDL_WINDOWEVENT) {
                gLastX = e.window.data1;
                gLastY = e.window.data2;
                if (e.window.data1 > 0)
                    gWidth = e.window.data1;
                if (e.window.data2 > 0)
                    gHeight = e.window.data2;
            }
        }
    }
