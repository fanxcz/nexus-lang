int64_t nexus_gfx_particle_create(double x,
                                  double y,
                                  double vx,
                                  double vy,
                                  double life,
                                  double size,
                                  double r,
                                  double g,
                                  double b,
                                  double a) {
    Particle p;
    p.x = float(x);
    p.y = float(y);
    p.vx = float(vx);
    p.vy = float(vy);
    p.life = float(std::max(life, 0.001));
    p.maxLife = p.life;
    p.size = float(size);
    p.r = float(r);
    p.g = float(g);
    p.b = float(b);
    p.a = float(a);
    int64_t id = gNextParticle++;
    gParticles[id] = p;
    return id;
}
bool nexus_gfx_particle_alive(int64_t id) {
    auto it = gParticles.find(id);
    return it != gParticles.end() && it->second.alive;
}
void nexus_gfx_particles_update(double dt) {
    float d = float(std::max(dt, 0.0));
    for (auto& [id, p] : gParticles) {
        if (!p.alive)
            continue;
        p.x += p.vx * d;
        p.y += p.vy * d;
        p.life -= d;
        if (p.life <= 0)
            p.alive = false;
    }
}
void nexus_gfx_particles_draw() {
    for (const auto& [id, p] : gParticles) {
        if (!p.alive)
            continue;
        float fade = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        draw_circle(p.x, p.y, p.size, p.r, p.g, p.b, p.a * fade);
    }
}
void nexus_gfx_particle_destroy(int64_t id) {
    gParticles.erase(id);
}
void nexus_gfx_particles_clear() {
    gParticles.clear();
}
