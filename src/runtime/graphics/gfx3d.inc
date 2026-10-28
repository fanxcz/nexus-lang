
// Simple OpenGL 2.1 compatible 3D backend.
void nexus_gfx3d_begin(double fov,
                       double nearPlane,
                       double farPlane,
                       double camX,
                       double camY,
                       double camZ,
                       double pitch,
                       double yaw,
                       double roll) {
    if (!gGraphics)
        return;
    if (!gOpenGL3D && !ensure_opengl_backend())
        return;
    glViewport(0, 0, gWidth, gHeight);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = double(gWidth) / double(gHeight ? gHeight : 1);
    double top = nearPlane * std::tan(fov * 3.14159265358979323846 / 360.0);
    double right = top * aspect;
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(float(-roll), 0, 0, 1);
    glRotatef(float(-pitch), 1, 0, 0);
    glRotatef(float(-yaw), 0, 1, 0);
    glTranslatef(float(-camX), float(-camY), float(-camZ));
}
void nexus_gfx3d_cube(double x,
                      double y,
                      double z,
                      double sx,
                      double sy,
                      double sz,
                      double r,
                      double g,
                      double b,
                      double a) {
    if (!gGraphics)
        return;
    glColor4f(float(r), float(g), float(b), float(a));
    float x0 = float(x - sx / 2), x1 = float(x + sx / 2), y0 = float(y - sy / 2),
          y1 = float(y + sy / 2), z0 = float(z - sz / 2), z1 = float(z + sz / 2);
    glBegin(GL_QUADS);

    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);
    glEnd();
}
void nexus_gfx3d_grid(int64_t cells, double spacing, double r, double g, double b, double a) {
    if (!gGraphics)
        return;
    glColor4f(float(r), float(g), float(b), float(a));
    glLineWidth(1);
    glBegin(GL_LINES);
    for (int64_t i = -cells; i <= cells; i++) {
        float p = float(i) * float(spacing);
        glVertex3f(p, 0, float(-cells * spacing));
        glVertex3f(p, 0, float(cells * spacing));
        glVertex3f(float(-cells * spacing), 0, p);
        glVertex3f(float(cells * spacing), 0, p);
    }
    glEnd();
}
void nexus_gfx3d_end() {
    if (!gOpenGL3D || !gWindow || !SDL_GL_SwapWindow)
        return;
    SDL_GL_SwapWindow(gWindow);
}

// Lightweight OBJ model loader for the fixed-function 3D backend.
int64_t nexus_gfx3d_model_load(const char* path) {

    if (!path) {
        gGraphicsError = "OBJ path is null";
        return 0;
    }
    std::ifstream f(path);
    if (!f) {
        gGraphicsError = std::string("cannot open OBJ: ") + path;
        return 0;
    }
    std::vector<std::array<float, 3>> pos;
    Model3D model;
    std::string line;

    while (std::getline(f, line)) {

        std::istringstream in(line);
        std::string tag;
        in >> tag;

        if (tag == "v") {
            float x, y, z;
            if (in >> x >> y >> z)
                pos.push_back({x, y, z});
        } else if (tag == "f") {

            std::vector<int> ids;
            std::string tok;

            while (in >> tok) {
                auto slash = tok.find('/');
                std::string n = tok.substr(0, slash);
                try {
                    int idx = std::stoi(n);
                    if (idx < 0)
                        idx = int(pos.size()) + idx + 1;
                    if (idx > 0 && idx <= int(pos.size()))
                        ids.push_back(idx - 1);
                } catch (...) {}
            }
            if (ids.size() >= 3) {
                for (size_t i = 1; i + 1 < ids.size(); ++i) {
                    for (int k : {ids[0], ids[i], ids[i + 1]}) {
                        auto v = pos[size_t(k)];
                        model.vertices.push_back(v[0]);
                        model.vertices.push_back(v[1]);
                        model.vertices.push_back(v[2]);
                    }
                }
            }
        }
    }
    if (model.vertices.empty()) {
        gGraphicsError = "OBJ contains no triangle faces";
        return 0;
    }
    int64_t id = gNextModel++;
    gModels[id] = std::move(model);
    return id;
}
void nexus_gfx3d_model_draw(int64_t id,
                            double x,
                            double y,
                            double z,
                            double sx,
                            double sy,
                            double sz,
                            double rx,
                            double ry,
                            double rz) {

    auto it = gModels.find(id);
    if (!gGraphics || it == gModels.end())
        return;

    glPushMatrix();
    glTranslatef(float(x), float(y), float(z));
    glRotatef(float(rx), 1, 0, 0);
    glRotatef(float(ry), 0, 1, 0);
    glRotatef(float(rz), 0, 0, 1);
    glScalef(float(sx), float(sy), float(sz));
    glColor4f(0.75f, 0.85f, 1.0f, 1.0f);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i + 2 < it->second.vertices.size(); i += 3)
        glVertex3f(it->second.vertices[i], it->second.vertices[i + 1], it->second.vertices[i + 2]);
    glEnd();
    glPopMatrix();
}
void nexus_gfx3d_model_unload(int64_t id) {
    gModels.erase(id);
}
