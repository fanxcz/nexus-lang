// Interactive terminal helpers remain available.
void nexus_screen_begin() {}
void nexus_screen_end() {}
void nexus_screen_clear() {
    nexus_clear();
}
void nexus_screen_put(int64_t x, int64_t y, const char* t) {
    std::printf("\033[%lld;%lldH%s", (long long)(y + 1), (long long)(x + 1), t ? t : "");
}
void nexus_screen_present() {
    std::fflush(stdout);
}
void nexus_screen_set_title(const char* t) {
    (void)t;
}
int64_t nexus_screen_width() {
    return 80;
}
int64_t nexus_screen_height() {
    return 24;
}
bool nexus_key_pressed() {
    return false;
}
int64_t nexus_read_key() {
    return -1;
}
void nexus_beep() {
    std::fputs("\a", stdout);
    std::fflush(stdout);
}
