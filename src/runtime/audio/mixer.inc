// WAV audio using SDL2 core audio queue. No SDL2_mixer or SDL_LoadWAV symbol is required.
int64_t nexus_gfx_sound_load(const char* path) {

    gAudioError.clear();

    if (!load_sdl() || !path || !SDL_OpenAudioDevice || !SDL_CloseAudioDevice ||
        !SDL_PauseAudioDevice || !SDL_QueueAudio || !SDL_GetQueuedAudioSize) {
        gAudioError = "SDL2 core audio API unavailable";
        gGraphicsError = gAudioError;
        return 0;
    }
    if (SDL_Init(0x00000010u) != 0) {
        gAudioError = std::string("SDL audio init failed: ") + sdl_error();
        gGraphicsError = gAudioError;
        return 0;
    }
    SDL_AudioSpec source{};
    std::vector<uint8_t> raw;
    std::string err;

    if (!load_pcm_wav(path, source, raw, err)) {
        gAudioError = err;
        gGraphicsError = err;
        return 0;
    }
    if (!ensure_audio_device(source)) {
        gGraphicsError = gAudioError;
        return 0;
    }
    std::vector<uint8_t> converted = convert_pcm(raw, source, gAudioSpec);

    if (converted.empty()) {
        gAudioError = "failed to convert WAV to the active audio device format";
        gGraphicsError = gAudioError;
        return 0;
    }
    Sound s;
    s.data = std::move(converted);
    s.freq = gAudioSpec.freq;
    s.format = gAudioSpec.format;
    s.channels = gAudioSpec.channels;
    s.samples = gAudioSpec.samples;

    int64_t id = gNextSound++;
    gSounds[id] = std::move(s);
    gAudio = true;
    return id;
}

void nexus_gfx_sound_tone(double frequency, int64_t durationMs, double volume) {

    gAudioError.clear();

    if (!load_sdl() || !SDL_OpenAudioDevice || !SDL_CloseAudioDevice || !SDL_PauseAudioDevice ||
        !SDL_QueueAudio) {
        gAudioError = "SDL2 audio queue API unavailable";
        return;
    }
    if (SDL_Init(0x00000010u) != 0) {
        gAudioError = std::string("SDL audio init failed: ") + sdl_error();
        return;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = 48000;
    wanted.format = 0x8010u;
    wanted.channels = 2;
    wanted.samples = 1024;

    if (!ensure_audio_device(wanted)) {
        return;
    }
    const double freq = std::max(20.0, std::min(20000.0, frequency));

    const int ms = std::max<int64_t>(8, std::min<int64_t>(durationMs, 2000));

    const int frames = int((int64_t(gAudioSpec.freq) * ms) / 1000);

    const int channels = std::max<int>(1, gAudioSpec.channels);

    const int bytes = (gAudioSpec.format == 0x0008u) ? 1 : (gAudioSpec.format == 0x8010u ? 2 : 0);

    if (bytes == 0) {
        gAudioError = "unsupported active audio format";
        return;
    }
    std::vector<uint8_t> data(size_t(frames) * size_t(channels) * size_t(bytes));

    double amp = std::clamp(volume, 0.0, 1.0);

    for (int i = 0; i < frames; ++i) {

        double t = double(i) / double(gAudioSpec.freq);

        double fadeIn = std::min(1.0, t / 0.006);

        double fadeOut = std::min(1.0, double(frames - i) / double(gAudioSpec.freq) * 18.0);

        double sample = std::sin(2.0 * 3.141592653589793 * freq * t) * amp * fadeIn * fadeOut;

        for (int c = 0; c < channels; ++c) {

            size_t at = (size_t(i) * size_t(channels) + size_t(c)) * size_t(bytes);

            if (bytes == 2) {
                int v = int(std::lrint(sample * 32767.0));
                data[at] = uint8_t(v & 0xff);
                data[at + 1] = uint8_t((uint16_t(v) >> 8) & 0xff);
            } else
                data[at] = uint8_t(std::clamp(128 + int(std::lrint(sample * 127.0)), 0, 255));
        }
    }
    if (SDL_ClearQueuedAudio)
        SDL_ClearQueuedAudio(gAudioDevice);

    if (SDL_QueueAudio(gAudioDevice, data.data(), uint32_t(data.size())) != 0)
        gAudioError = "SDL_QueueAudio failed";

    else
        gAudio = true;
}
void nexus_gfx_sound_play(int64_t id, bool loop) {

    auto it = gSounds.find(id);
    if (it == gSounds.end()) {
        gAudioError = "unknown sound id";
        return;
    }
    if (!gAudioDevice) {
        gAudioError = "audio device is not open";
        return;
    }
    const Sound& s = it->second;

    std::vector<uint8_t> scaled = s.data;

    if (gAudioVolume < 128) {

        if (s.format == 0x8010u) {

            for (size_t i = 0; i + 1 < scaled.size(); i += 2) {
                int16_t sample = int16_t(uint16_t(scaled[i]) | (uint16_t(scaled[i + 1]) << 8));
                sample = int16_t(std::clamp(int(sample) * gAudioVolume / 128, -32768, 32767));
                scaled[i] = uint8_t(sample & 0xff);
                scaled[i + 1] = uint8_t((uint16_t(sample) >> 8) & 0xff);
            }
        } else if (s.format == 0x0008u) {

            for (auto& v : scaled) {
                int sample = int(v) - 128;
                sample = std::clamp(sample * gAudioVolume / 128, -128, 127);
                v = uint8_t(sample + 128);
            }
        }
    }
    if (!SDL_QueueAudio(gAudioDevice, scaled.data(), uint32_t(scaled.size()))) {
        gAudioError = "SDL_QueueAudio failed";
        gGraphicsError = gAudioError;
        return;
    }
    if (loop) {
        for (int i = 0; i < 63; i++)
            SDL_QueueAudio(gAudioDevice, scaled.data(), uint32_t(scaled.size()));
    }
}
void nexus_gfx_sound_stop() {
    if (gAudioDevice) {
        if (SDL_ClearQueuedAudio)
            SDL_ClearQueuedAudio(gAudioDevice);
        SDL_CloseAudioDevice(gAudioDevice);
        gAudioDevice = 0;
    }
    gAudio = false;
}
void nexus_gfx_sound_unload(int64_t id) {
    gSounds.erase(id);
}
int64_t nexus_gfx_audio_available() {
    // Availability means an audio device is actually open, not just that the
    // SDL shared library can be loaded. This keeps headless hosts accurate.
    return gAudioDevice != 0 ? 1 : 0;
}
const char* nexus_gfx_audio_error() {
    return gAudioError.empty() ? "" : gAudioError.c_str();
}
void nexus_gfx_sound_volume(double v) {
    gAudioVolume = std::clamp(int(v * 128.0), 0, 128);
}
bool nexus_gfx_sound_playing() {
    return gAudioDevice && SDL_GetQueuedAudioSize && SDL_GetQueuedAudioSize(gAudioDevice) > 0;
}
