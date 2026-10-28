    static std::vector<uint8_t> convert_pcm(const std::vector<uint8_t>& src,
                                            const SDL_AudioSpec& in,
                                            const SDL_AudioSpec& out) {

        const int inBytes = (in.format == 0x0008u) ? 1
                            : (in.format == 0x8010u) ? 2
                            : (in.format == 0x8120u) ? 4
                                                     : 0;

        const int outBytes = (out.format == 0x0008u) ? 1
                             : (out.format == 0x8010u) ? 2
                             : (out.format == 0x8120u) ? 4
                                                       : 0;

        if (inBytes == 0 || outBytes == 0 || in.channels == 0 || out.channels == 0 ||
            in.freq <= 0 || out.freq <= 0)
            return {};

        const size_t inFrameBytes = size_t(inBytes) * in.channels;

        if (inFrameBytes == 0 || src.size() < inFrameBytes)
            return {};

        const size_t inFrames = src.size() / inFrameBytes;

        const size_t outFrames = std::max<size_t>(
            1, size_t(std::llround(double(inFrames) * double(out.freq) / double(in.freq))));

        std::vector<float> inF(inFrames * in.channels, 0.0f);

        for (size_t f = 0; f < inFrames; ++f) {

            for (unsigned c = 0; c < in.channels; ++c)
                inF[f * in.channels + c] =
                    pcm_sample(src.data() + f * inFrameBytes + c * inBytes, in.format);
        }
        std::vector<float> outF(outFrames * out.channels, 0.0f);

        for (size_t of = 0; of < outFrames; ++of) {

            double srcPos = double(of) * double(in.freq) / double(out.freq);

            size_t i0 = size_t(std::min<double>(std::floor(srcPos), double(inFrames - 1)));

            size_t i1 = std::min(i0 + 1, inFrames - 1);

            float frac = float(srcPos - double(i0));

            auto chan = [&](unsigned c) -> float {
                if (in.channels == 1)
                    return inF[i0] * (1.0f - frac) + inF[i1] * frac;

                if (out.channels == 1) {

                    float a0 = inF[i0 * in.channels + 0],
                          a1 = inF[i0 * in.channels + std::min<unsigned>(1, in.channels - 1)];

                    float b0 = inF[i1 * in.channels + 0],
                          b1 = inF[i1 * in.channels + std::min<unsigned>(1, in.channels - 1)];

                    return ((a0 + a1) * 0.5f) * (1.0f - frac) + ((b0 + b1) * 0.5f) * frac;
                }
                unsigned srcC = std::min<unsigned>(c, in.channels - 1);

                return inF[i0 * in.channels + srcC] * (1.0f - frac) +
                       inF[i1 * in.channels + srcC] * frac;
            };

            for (unsigned c = 0; c < out.channels; ++c)
                outF[of * out.channels + c] = std::clamp(chan(c), -1.0f, 1.0f);
        }
        std::vector<uint8_t> converted(outF.size() * size_t(outBytes));

        for (size_t i = 0; i < outF.size(); ++i) {

            if (out.format == 0x8120u) {

                float v = outF[i];
                std::memcpy(converted.data() + i * 4, &v, sizeof(v));

            } else if (out.format == 0x8010u) {

                int v = int(std::lrint(std::clamp(outF[i], -1.0f, 1.0f) * 32767.0f));

                converted[i * 2] = uint8_t(v & 0xff);
                converted[i * 2 + 1] = uint8_t((uint16_t(v) >> 8) & 0xff);

            } else {

                int v = 128 + int(std::lrint(std::clamp(outF[i], -1.0f, 1.0f) * 127.0f));

                converted[i] = uint8_t(std::clamp(v, 0, 255));
            }
        }
        return converted;
    }
