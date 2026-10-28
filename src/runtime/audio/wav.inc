    static uint16_t rd16(const uint8_t* p) {
        return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
    }
    static uint32_t rd32(const uint8_t* p) {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
               (uint32_t(p[3]) << 24);
    }
    static bool load_pcm_wav(const char* path,
                             SDL_AudioSpec& spec,
                             std::vector<uint8_t>& audio,
                             std::string& err) {

        std::FILE* f = std::fopen(path, "rb");

        if (!f) {
            err = std::string("cannot open WAV: ") + path;
            return false;
        }
        std::vector<uint8_t> bytes;

        std::fseek(f, 0, SEEK_END);

        long n = std::ftell(f);

        std::rewind(f);

        if (n <= 0) {
            std::fclose(f);
            err = "WAV file is empty";
            return false;
        }
        bytes.resize(size_t(n));

        if (std::fread(bytes.data(), 1, bytes.size(), f) != bytes.size()) {
            std::fclose(f);
            err = "cannot read WAV";
            return false;
        }
        std::fclose(f);

        if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
            std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
            err = "not a RIFF/WAVE file";
            return false;
        }
        uint16_t audioFormat = 0, channels = 0, bits = 0;

        uint32_t sampleRate = 0, dataOffset = 0, dataSize = 0;

        size_t pos = 12;

        while (pos + 8 <= bytes.size()) {

            const uint8_t* h = bytes.data() + pos;

            uint32_t chunkSize = rd32(h + 4);

            size_t payload = pos + 8;

            if (payload > bytes.size())
                break;

            size_t available = bytes.size() - payload;

            size_t actual = std::min<size_t>(chunkSize, available);

            if (std::memcmp(h, "fmt ", 4) == 0 && actual >= 16) {

                const uint8_t* q = bytes.data() + payload;

                audioFormat = rd16(q + 0);

                channels = rd16(q + 2);

                sampleRate = rd32(q + 4);

                bits = rd16(q + 14);

            } else if (std::memcmp(h, "data", 4) == 0) {

                dataOffset = uint32_t(payload);

                dataSize = uint32_t(actual);
            }
            pos = payload + actual + (actual & 1u);
        }
        if (audioFormat != 1) {
            err = "only PCM WAV is supported without SDL2_mixer";
            return false;
        }
        if (channels == 0 || channels > 2 || sampleRate == 0) {
            err = "unsupported WAV channel/sample-rate configuration";
            return false;
        }
        if (bits != 8 && bits != 16) {
            err = "only 8-bit or 16-bit PCM WAV is supported";
            return false;
        }
        if (dataSize == 0 || dataOffset + dataSize > bytes.size()) {
            err = "WAV data chunk is missing or invalid";
            return false;
        }
        spec = SDL_AudioSpec{};

        spec.freq = int(sampleRate);

        spec.channels = uint8_t(channels);

        spec.samples = 4096;

        spec.padding = 0;

        spec.callback = nullptr;

        spec.userdata = nullptr;

        if (bits == 8)
            spec.format = 0x0008u;
        // AUDIO_U8
        else
            spec.format = 0x8010u;
        // AUDIO_S16LSB
        spec.size = dataSize;

        audio.assign(bytes.begin() + dataOffset, bytes.begin() + dataOffset + dataSize);

        return true;
    }

    static bool ensure_audio_device(const SDL_AudioSpec& wanted) {

        if (gAudioDevice)
            return true;

        if (!SDL_OpenAudioDevice || !SDL_PauseAudioDevice) {
            gAudioError = "SDL2 audio queue API unavailable";
            return false;
        }
        SDL_AudioSpec obtained{};

        constexpr int ALLOW_FREQUENCY_CHANGE = 0x0001;

        constexpr int ALLOW_FORMAT_CHANGE = 0x0002;

        constexpr int ALLOW_CHANNELS_CHANGE = 0x0004;

        const int allowed = ALLOW_FREQUENCY_CHANGE | ALLOW_FORMAT_CHANGE | ALLOW_CHANNELS_CHANGE;

        gAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, allowed);

        if (!gAudioDevice) {
            gAudioError = std::string("SDL_OpenAudioDevice failed: ") + sdl_error();
            return false;
        }
        gAudioSpec = obtained;

        SDL_PauseAudioDevice(gAudioDevice, 0);

        return true;
    }

    static float pcm_sample(const uint8_t* p, uint16_t format) {

        if (format == 0x0008u)
            return (float(int(*p) - 128)) / 128.0f;

        if (format == 0x8010u) {
            int16_t v = int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
            return float(v) / 32768.0f;
        }

        if (format == 0x8120u) {
            float v = 0.0f;
            std::memcpy(&v, p, sizeof(v));
            return std::clamp(v, -1.0f, 1.0f);
        }
        return 0.0f;
    }
