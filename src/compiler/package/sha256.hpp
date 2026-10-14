#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace nexus::package {

    // Dependency-free SHA-256 for local package integrity metadata. A hash detects
    // content changes; it does not authenticate publishers or make archives trusted.
    inline std::string sha256Hex(std::string_view input) {
        static constexpr std::uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
            0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
            0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
            0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
            0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
            0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
            0xc67178f2};

        std::uint32_t state[8] = {0x6a09e667,
                                  0xbb67ae85,
                                  0x3c6ef372,
                                  0xa54ff53a,
                                  0x510e527f,
                                  0x9b05688c,
                                  0x1f83d9ab,
                                  0x5be0cd19};
        const auto rotr = [](std::uint32_t x, unsigned n) { return (x >> n) | (x << (32U - n)); };
        const auto transform = [&](const std::uint8_t* block) {
            std::uint32_t w[64]{};
            for (std::size_t i = 0; i < 16; ++i) {
                const std::size_t j = i * 4;
                w[i] = (std::uint32_t(block[j]) << 24) | (std::uint32_t(block[j + 1]) << 16) |
                       (std::uint32_t(block[j + 2]) << 8) | std::uint32_t(block[j + 3]);
            }
            for (std::size_t i = 16; i < 64; ++i) {
                const std::uint32_t s0 =
                    rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
            std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
            for (std::size_t i = 0; i < 64; ++i) {
                const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                const std::uint32_t choose = (e & f) ^ (~e & g);
                const std::uint32_t t1 = h + s1 + choose + k[i] + w[i];
                const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
                const std::uint32_t t2 = s0 + majority;
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
            }
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;
        };

        const auto* bytes = reinterpret_cast<const std::uint8_t*>(input.data());
        const std::size_t complete = input.size() / 64;
        for (std::size_t i = 0; i < complete; ++i)
            transform(bytes + i * 64);

        std::uint8_t finalBlock[128]{};
        const std::size_t used = complete * 64;
        const std::size_t tail = input.size() - used;
        if (tail)
            std::memcpy(finalBlock, bytes + used, tail);
        finalBlock[tail] = 0x80;

        const std::uint64_t bitLength = static_cast<std::uint64_t>(input.size()) * 8U;
        const std::size_t finalSize = tail < 56 ? 64 : 128;
        for (std::size_t i = 0; i < 8; ++i)
            finalBlock[finalSize - 1 - i] =
                static_cast<std::uint8_t>(bitLength >> (8U * static_cast<unsigned>(i)));
        transform(finalBlock);
        if (finalSize == 128)
            transform(finalBlock + 64);

        static constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(64);
        for (const auto word : state) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                const auto byte = static_cast<std::uint8_t>(word >> shift);
                result.push_back(digits[byte >> 4]);
                result.push_back(digits[byte & 0x0f]);
            }
        }
        return result;
    }

} // namespace nexus::package
