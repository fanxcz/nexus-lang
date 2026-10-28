    // glBlendFunc is loaded separately to keep the backend tiny but complete.
    using PFN_glBlendFunc = void (*)(GLenum, GLenum);

    static PFN_glBlendFunc glBlendFuncPtr = nullptr;

    static void setup_2d() {

        const double logicalWidth = gUiWidth > 0.0 ? gUiWidth : static_cast<double>(gWidth);

        const double logicalHeight = gUiHeight > 0.0 ? gUiHeight : static_cast<double>(gHeight);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);

        if (glBlendFuncPtr) {

            glBlendFuncPtr(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glMatrixMode(GL_PROJECTION);

        glLoadIdentity();

        glOrtho(0.0, logicalWidth, logicalHeight, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();

        if (gCameraZoom != 1.0f || gCameraX != 0.0f || gCameraY != 0.0f) {

            glTranslatef(static_cast<float>(logicalWidth * 0.5),
                         static_cast<float>(logicalHeight * 0.5),
                         0.0f);

            glScalef(gCameraZoom, gCameraZoom, 1.0f);

            glTranslatef(static_cast<float>(-logicalWidth * 0.5 - gCameraX),
                         static_cast<float>(-logicalHeight * 0.5 - gCameraY),
                         0.0f);
        }
    }

    static void draw_quad(float x, float y, float w, float h, float r, float g, float b, float a) {

        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    }

    static void draw_circle(float cx, float cy, float radius, float r, float g, float b, float a) {

        glColor4f(r, g, b, a);
        glBegin(GL_TRIANGLES);
        constexpr int N = 48;
        for (int i = 0; i < N; i++) {
            double a0 = (2.0 * M_PI * i) / N, a1 = (2.0 * M_PI * (i + 1)) / N;
            glVertex2f(cx, cy);
            glVertex2f(float(cx + std::cos(a0) * radius), float(cy + std::sin(a0) * radius));
            glVertex2f(float(cx + std::cos(a1) * radius), float(cy + std::sin(a1) * radius));
        }
        glEnd();
    }
