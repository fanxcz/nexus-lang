#pragma once

#include "native/image.hpp"

#include <cstdint>
#include <string>

namespace nexus::native {
    namespace detail {

        // Minimal x86-64 instruction encoder. Every method appends bytes to a
        // CodeImage; labels and fixups are resolved by the object writer.
        class Emitter {
        public:
            explicit Emitter(CodeImage& image) : image_(image) {}

            std::size_t pos() const {
                return image_.bytes.size();
            }
            void def(const std::string& name) {
                image_.labels[name] = pos();
            }

            void b(std::uint8_t v) {
                image_.bytes.push_back(v);
            }
            void i32(std::uint32_t v) {
                for (int i = 0; i < 4; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }
            void i64(std::uint64_t v) {
                for (int i = 0; i < 8; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }

            // 32-bit relative fixup: field = symbol - (codeBase + at + 4) + addend.
            void rel(const std::string& symbol, std::int32_t addend = 0) {
                image_.fixups.push_back({pos(), symbol, addend});
                i32(0);
            }

            std::string label(const std::string& prefix) {
                return prefix + "$" + std::to_string(nextLabel_++);
            }

            void pushRbp() {
                b(0x55);
            }
            void pushRax() {
                b(0x50);
            }
            void pushRbx() {
                b(0x53);
            }
            void popRax() {
                b(0x58);
            }
            void popRcx() {
                b(0x59);
            }
            void popRbx() {
                b(0x5B);
            }
            void popRbp() {
                b(0x5D);
            }
            void movRbxRsp() {
                b(0x48);
                b(0x89);
                b(0xE3);
            }
            void movRspRbx() {
                b(0x48);
                b(0x89);
                b(0xDC);
            }
            void ret() {
                b(0xC3);
            }
            void ud2() {
                b(0x0F);
                b(0x0B);
            }
            void repMovsb() {
                b(0xF3);
                b(0xA4);
            }
            void movRbpRsp() {
                b(0x48);
                b(0x89);
                b(0xE5);
            }
            void movRspRbp() {
                b(0x48);
                b(0x89);
                b(0xEC);
            }
            void movR11Rsp() {
                b(0x49);
                b(0x89);
                b(0xE3);
            }
            void movRspR11() {
                b(0x4C);
                b(0x89);
                b(0xDC);
            }
            void subRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xEC);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xEC);
                    i32(n);
                }
            }
            void addRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xC4);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xC4);
                    i32(n);
                }
            }
            void andRspMinus16() {
                b(0x48);
                b(0x83);
                b(0xE4);
                b(0xF0);
            }
            void movRaxImm64(std::uint64_t v) {
                b(0x48);
                b(0xB8);
                i64(v);
            }
            void movEcxImm32(std::uint32_t v) {
                b(0xB9);
                i32(v);
            }
            void movEaxImm32(std::uint32_t v) {
                b(0xB8);
                i32(v);
            }
            void movEdxImm32(std::uint32_t v) {
                b(0xBA);
                i32(v);
            }
            void movEdiEax() {
                b(0x89);
                b(0xC7);
            }
            void movEcxEax() {
                b(0x89);
                b(0xC1);
            }
            void movRcxRax() {
                b(0x48);
                b(0x89);
                b(0xC1);
            }
            void movRdxRax() {
                b(0x48);
                b(0x89);
                b(0xC2);
            }
            void movRsiRcx() {
                b(0x48);
                b(0x89);
                b(0xCE);
            }
            void movRaxRdx() {
                b(0x48);
                b(0x89);
                b(0xD0);
            }
            void movRaxRdi() {
                b(0x48);
                b(0x89);
                b(0xF8);
            }
            void movRaxRsi() {
                b(0x48);
                b(0x89);
                b(0xF0);
            }
            void movRsiRax() {
                b(0x48);
                b(0x89);
                b(0xC6);
            }
            void movRsiRdx() {
                b(0x48);
                b(0x89);
                b(0xD6);
            }
            void movRaxR8() {
                b(0x4C);
                b(0x89);
                b(0xC0);
            }
            void movRdxRsi() {
                b(0x48);
                b(0x89);
                b(0xF2);
            }
            void movRcxRdx() {
                b(0x48);
                b(0x89);
                b(0xD1);
            }
            void movRcxRsi() {
                b(0x48);
                b(0x89);
                b(0xF1);
            }
            void movRcxR8() {
                b(0x4C);
                b(0x89);
                b(0xC1);
            }
            void movRcxR9() {
                b(0x4C);
                b(0x89);
                b(0xC9);
            }
            void movRcxR11() {
                b(0x4C);
                b(0x89);
                b(0xD9);
            }
            void movRdiRsi() {
                b(0x48);
                b(0x89);
                b(0xF7);
            }
            void movRsiR11() {
                b(0x4C);
                b(0x89);
                b(0xDE);
            }
            void movEaxFromRsp8(std::uint8_t d) {
                b(0x8B);
                b(0x44);
                b(0x24);
                b(d);
            }
            void movRsiImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC6);
                i32(v);
            }
            void movRdxImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC2);
                i32(v);
            }
            void movR8Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC0);
                i32(v);
            }
            void movR9Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC1);
                i32(v);
            }
            void movR10Imm32(std::uint32_t v) {
                b(0x41);
                b(0xBA);
                i32(v);
            }
            void movRcxImm10() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(10);
            }
            void movR10Imm1() {
                b(0x41);
                b(0xBA);
                i32(1);
            }
            void xorEdx() {
                b(0x31);
                b(0xD2);
            }
            void xorEax() {
                b(0x31);
                b(0xC0);
            }
            void xorRax() {
                b(0x48);
                b(0x31);
                b(0xC0);
            }
            void xorEcx() {
                b(0x31);
                b(0xC9);
            }
            void xorEdi() {
                b(0x31);
                b(0xFF);
            }
            void xorR8d() {
                b(0x45);
                b(0x31);
                b(0xC0);
            }
            void xorR9d() {
                b(0x45);
                b(0x31);
                b(0xC9);
            }
            void xorR10d() {
                b(0x45);
                b(0x31);
                b(0xD2);
            }
            void addRaxImm32(std::uint32_t v) {
                b(0x48);
                b(0x05);
                i32(v);
            }
            void addRaxRcx() {
                b(0x48);
                b(0x01);
                b(0xC8);
            }
            void addRaxRdi() {
                b(0x48);
                b(0x01);
                b(0xF8);
            }
            void addRaxRsi() {
                b(0x48);
                b(0x01);
                b(0xF0);
            }
            void addRaxR9() {
                b(0x4C);
                b(0x01);
                b(0xC8);
            }
            void addRsiRdx() {
                b(0x48);
                b(0x01);
                b(0xD6);
            }
            void addRsiRcx() {
                b(0x48);
                b(0x01);
                b(0xCE);
            }
            void addR11Imm32(std::uint32_t v) {
                b(0x49);
                b(0x81);
                b(0xC3);
                i32(v);
            }
            void incRax() {
                b(0x48);
                b(0xFF);
                b(0xC0);
            }
            void incRcx() {
                b(0x48);
                b(0xFF);
                b(0xC1);
            }
            void incRdx() {
                b(0x48);
                b(0xFF);
                b(0xC2);
            }
            void incRsi() {
                b(0x48);
                b(0xFF);
                b(0xC6);
            }
            void incR8() {
                b(0x49);
                b(0xFF);
                b(0xC0);
            }
            void incR9() {
                b(0x49);
                b(0xFF);
                b(0xC1);
            }
            void decEcx() {
                b(0xFF);
                b(0xC9);
            }
            void decR8() {
                b(0x49);
                b(0xFF);
                b(0xC8);
            }
            void subRaxRcx() {
                b(0x48);
                b(0x29);
                b(0xC8);
            }
            void imulRaxRcx() {
                b(0x48);
                b(0x0F);
                b(0xAF);
                b(0xC1);
            }
            void cqo() {
                b(0x48);
                b(0x99);
            }
            void idivRcx() {
                b(0x48);
                b(0xF7);
                b(0xF9);
            }
            void divRcx() {
                b(0x48);
                b(0xF7);
                b(0xF1);
            }
            void negRax() {
                b(0x48);
                b(0xF7);
                b(0xD8);
            }
            void notBoolRax() {
                b(0x48);
                b(0x83);
                b(0xF0);
                b(0x01);
            }
            void andRaxRcx() {
                b(0x48);
                b(0x21);
                b(0xC8);
            }
            void orRaxRcx() {
                b(0x48);
                b(0x09);
                b(0xC8);
            }
            void xorRaxRcx() {
                b(0x48);
                b(0x31);
                b(0xC8);
            }
            void shlRaxCl() {
                b(0x48);
                b(0xD3);
                b(0xE0);
            }
            void sarRaxCl() {
                b(0x48);
                b(0xD3);
                b(0xF8);
            }
            void cmpRaxRcx() {
                b(0x48);
                b(0x39);
                b(0xC8);
            }
            void cmpRcxRax() {
                b(0x48);
                b(0x39);
                b(0xC1);
            }
            void cmpRaxR11() {
                b(0x4C);
                b(0x39);
                b(0xD8);
            }
            void cmpRaxImm8(std::uint8_t v) {
                b(0x48);
                b(0x83);
                b(0xF8);
                b(v);
            }
            void cmpBytePtrRcx(std::uint8_t v) {
                b(0x80);
                b(0x39);
                b(v);
            }
            void cmpBytePtrRsi(std::uint8_t v) {
                b(0x80);
                b(0x3E);
                b(v);
            }
            void cmpAlDl() {
                b(0x38);
                b(0xC2);
            }
            void cmpAlImm10() {
                b(0x3C);
                b(0x0A);
            }
            void cmpAlImm13() {
                b(0x3C);
                b(0x0D);
            }
            void cmpRdxRax() {
                b(0x48);
                b(0x39);
                b(0xC2);
            }
            void testRaxRax() {
                b(0x48);
                b(0x85);
                b(0xC0);
            }
            void testRdxRdx() {
                b(0x48);
                b(0x85);
                b(0xD2);
            }
            void testRcxRcx() {
                b(0x48);
                b(0x85);
                b(0xC9);
            }
            void testEcxEcx() {
                b(0x85);
                b(0xC9);
            }
            void testAlAl() {
                b(0x84);
                b(0xC0);
            }
            void testR8R8() {
                b(0x4D);
                b(0x85);
                b(0xC0);
            }
            void testR10d() {
                b(0x45);
                b(0x85);
                b(0xD2);
            }
            void setcc(std::uint8_t cc) {
                b(0x0F);
                b(cc);
                b(0xC0);
            }
            void movzxEaxAl() {
                b(0x0F);
                b(0xB6);
                b(0xC0);
            }
            void addDl48() {
                b(0x80);
                b(0xC2);
                b(0x30);
            }
            void decRsi() {
                b(0x48);
                b(0xFF);
                b(0xCE);
            }
            void subRdxRsi() {
                b(0x48);
                b(0x29);
                b(0xF2);
            }
            void syscall() {
                b(0x0F);
                b(0x05);
            }
            void cld() {
                b(0xFC);
            }
            void repneScasb() {
                b(0xF2);
                b(0xAE);
            }
            void repeCmpsb() {
                b(0xF3);
                b(0xA6);
            }
            void notRcx() {
                b(0x48);
                b(0xF7);
                b(0xD1);
            }
            void decRcx() {
                b(0x48);
                b(0xFF);
                b(0xC9);
            }
            void movRdiRcx() {
                b(0x48);
                b(0x89);
                b(0xCF);
            }
            void movRcxImmNeg1() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(0xFFFFFFFFu);
            }
            void storeByteRsiImm8(std::uint8_t v) {
                b(0xC6);
                b(0x06);
                b(v);
            }
            void storeByteRsiImm0() {
                b(0xC6);
                b(0x06);
                b(0x00);
            }
            void storeByteRdiImm0() {
                b(0xC6);
                b(0x07);
                b(0x00);
            }
            void storeDlRsi() {
                b(0x88);
                b(0x16);
            }
            void movAlFromRcx() {
                b(0x8A);
                b(0x01);
            }
            void movAlFromRsi() {
                b(0x8A);
                b(0x06);
            }
            void movAlFromRsiMinus1() {
                b(0x8A);
                b(0x46);
                b(0xFF);
            }
            void movDlFromRdx() {
                b(0x8A);
                b(0x12);
            }
            void movEdiImm32(std::uint32_t v) {
                b(0xBF);
                i32(v);
            }
            void leaR9FromRsp8(std::uint8_t d) {
                b(0x4C);
                b(0x8D);
                b(0x4C);
                b(0x24);
                b(d);
            }
            void storeQwordRsp32Imm0() {
                b(0x48);
                b(0xC7);
                b(0x44);
                b(0x24);
                b(0x20);
                i32(0);
            }
            void jmp(const std::string& target) {
                b(0xE9);
                rel(target);
            }
            void jcc(std::uint8_t cc, const std::string& target) {
                b(0x0F);
                b(cc);
                rel(target);
            }
            void call(const std::string& target) {
                b(0xE8);
                rel(target);
            }
            void callIAT(const std::string& importSym) {
                b(0xFF);
                b(0x15);
                rel(importSym);
            }
            void leaRcxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x0D);
                rel(s);
            }
            void leaRaxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x05);
                rel(s);
            }
            void leaR10Rip(const std::string& s) {
                b(0x4C);
                b(0x8D);
                b(0x15);
                rel(s);
            }
            // Arena word access: __nx_arena[0] = base, __nx_arena[1] = pos.
            void movR11FromR10() {
                b(0x4D);
                b(0x8B);
                b(0x1A);
            }
            void movR11FromR10Disp8(std::uint8_t d) {
                b(0x4D);
                b(0x8B);
                b(0x5A);
                b(d);
            }
            void movRdiFromR10Disp8(std::uint8_t d) {
                b(0x49);
                b(0x8B);
                b(0x7A);
                b(d);
            }
            void movR10FromRax() {
                b(0x49);
                b(0x89);
                b(0x02);
            }
            void movR10Disp8FromRax(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x42);
                b(d);
            }
            void movR10Disp8FromRsi(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x72);
                b(d);
            }
            void leaRdxFromRbp(std::int64_t off) {
                leaFromRbp(0x55, 0x95, off);
            }
            void leaRsiFromRbp(std::int64_t off) {
                leaFromRbp(0x75, 0xB5, off);
            }

            // rbp-relative loads/stores. Negative offsets address locals.
            void movRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x45, 0x85, off);
            }
            void subRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x2B, 0x45, 0x85, off);
            }
            void movRsiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x75, 0xB5, off);
            }
            void movRbpFromRsi(std::int64_t off) {
                memToReg(0x48, 0x89, 0x75, 0xB5, off);
            }
            void movRbpFromRax(std::int64_t off) {
                memToReg(0x48, 0x89, 0x45, 0x85, off);
            }
            void movRcxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x4D, 0x8D, off);
            }
            void movRdxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x55, 0x95, off);
            }
            void movRdiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x7D, 0xBD, off);
            }
            void movR8FromRbp(std::int64_t off) {
                memToReg(0x4C, 0x8B, 0x45, 0x85, off);
            }
            void movRcxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x4D, 0x8D, off);
            }
            void movRdxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x55, 0x95, off);
            }
            void
            movR11FromRbp(std::int64_t off) { // Positive offsets address incoming stack arguments.
                if (off >= 0) {
                    if (off <= 127) {
                        b(0x4C);
                        b(0x8B);
                        b(0x5D);
                        b(static_cast<std::uint8_t>(off));
                    } else {
                        b(0x4C);
                        b(0x8B);
                        b(0x9D);
                        i32(static_cast<std::uint32_t>(off));
                    }
                    return;
                }
                memToReg(0x4C, 0x8B, 0x5D, 0x9D, off);
            }
            void movRbpFromR11(std::int64_t off) {
                memToReg(0x4C, 0x89, 0x5D, 0x9D, off);
            }

        private:
            void memToReg(std::uint8_t rex,
                          std::uint8_t op,
                          std::uint8_t modrm8,
                          std::uint8_t modrm32,
                          std::int64_t off) {
                b(rex);
                b(op);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }
            void leaFromRbp(std::uint8_t modrm8, std::uint8_t modrm32, std::int64_t off) {
                b(0x48);
                b(0x8D);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }

            CodeImage& image_;
            std::size_t nextLabel_ = 0;
        };

        // Condition codes (0F-prefixed setcc/jcc second byte).
        constexpr std::uint8_t CC_E = 0x94, CC_NE = 0x95, CC_L = 0x9C, CC_LE = 0x9E;
        constexpr std::uint8_t CC_G = 0x9F, CC_GE = 0x9D;
        constexpr std::uint8_t J_E = 0x84, J_NE = 0x85, J_NS = 0x89;
        constexpr std::uint8_t J_L = 0x8C, J_AE = 0x83;
        constexpr std::uint8_t J_A = 0x87, J_LE = 0x8E;
        constexpr std::uint32_t kArenaSize = 0x1000000; // 16 MiB bump arena for strings

    } // namespace detail
} // namespace nexus::native
