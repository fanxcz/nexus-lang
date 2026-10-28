bool nexus_physics_aabb(
    double ax, double ay, double aw, double ah, double bx, double by, double bw, double bh) {
    // Reject invalid geometry and overflowing derived edges before comparing bounds.
    if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(aw) ||
        !std::isfinite(ah) || !std::isfinite(bx) || !std::isfinite(by) ||
        !std::isfinite(bw) || !std::isfinite(bh) ||
        aw <= 0.0 || ah <= 0.0 || bw <= 0.0 || bh <= 0.0)
        return false;

    const double ar = ax + aw;
    const double ab = ay + ah;
    const double br = bx + bw;
    const double bb = by + bh;
    if (!std::isfinite(ar) || !std::isfinite(ab) ||
        !std::isfinite(br) || !std::isfinite(bb))
        return false;

    return ax < br && ar > bx && ay < bb && ab > by;
}
bool nexus_physics_circle(double ax, double ay, double ar, double bx, double by, double br) {
    if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(ar) ||
        !std::isfinite(bx) || !std::isfinite(by) || !std::isfinite(br) ||
        ar < 0.0 || br < 0.0)
        return false;

    const double dx = ax - bx;
    const double dy = ay - by;
    const double radius = ar + br;
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(radius))
        return false;

    // hypot avoids an Inf <= Inf result when squaring large finite distances.
    return std::hypot(dx, dy) <= radius;
}
