#include "native/native_generator.hpp"

#include <stdexcept>

namespace nexus::native {
    namespace detail {


            // __nx_write: rcx = pointer, rdx = length. Writes to stdout.
        void Generator::genWrite() {
            out_.def("__nx_write");
            if (target_ == NativeTarget::WindowsX64) {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.movRcxToRbp(-8);
                out_.movRdxToRbp(-16);
                // handle = GetStdHandle(STD_OUTPUT_HANDLE = -11)
                out_.movEcxImm32(static_cast<std::uint32_t>(-11));
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-24);
                // WriteFile(handle, ptr, len, NULL, NULL)
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-8);
                out_.movR8FromRbp(-16);
                out_.xorR9d();
                out_.subRsp(48);
                out_.storeQwordRsp32Imm0();
                out_.callIAT("__iat_WriteFile");
                out_.addRsp(48);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            } else {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiEax();    // fd = 1 (stdout)
                out_.syscall();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }
        }

            // __nx_print_i64: rax = value. Prints decimal + newline.
        void Generator::genPrintI64() {
            out_.def("__nx_print_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.leaRsiFromRbp(-48);
            out_.decRsi();
            out_.storeByteRsiImm8(0x0A);
            out_.xorR10d();
            out_.testRaxRax();
            const std::string positive = out_.label("i64.pos");
            out_.jcc(J_NS, positive);
            out_.negRax();
            out_.movR10Imm1();
            out_.def(positive);
            out_.movRcxImm10();
            const std::string digit = out_.label("i64.digit");
            out_.def(digit);
            out_.xorEdx();
            out_.divRcx();
            out_.addDl48();
            out_.decRsi();
            out_.storeDlRsi();
            out_.testRaxRax();
            out_.jcc(J_NE, digit);
            out_.testR10d();
            const std::string noSign = out_.label("i64.nosign");
            out_.jcc(J_E, noSign);
            out_.decRsi();
            out_.storeByteRsiImm8(0x2D);
            out_.def(noSign);
            out_.leaRdxFromRbp(-48);
            out_.subRdxRsi();
            out_.movRcxRsi();
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_print_bool: rax = 0/1. Prints "true"/"false" + newline.
        void Generator::genPrintBool() {
            const std::size_t t = internString("true\n");
            const std::size_t f = internString("false\n");
            out_.def("__nx_print_bool");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.testRaxRax();
            const std::string isFalse = out_.label("bool.false");
            const std::string go = out_.label("bool.go");
            out_.jcc(J_E, isFalse);
            out_.leaRcxRip("str" + std::to_string(t));
            out_.movEdxImm32(5);
            out_.jmp(go);
            out_.def(isFalse);
            out_.leaRcxRip("str" + std::to_string(f));
            out_.movEdxImm32(6);
            out_.def(go);
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_write_cstr: rax = NUL-terminated string. Writes it + newline.
        void Generator::genWriteCstr() {
            const std::size_t nl = internString("\n");
            out_.def("__nx_write_cstr");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.movRcxRax();
            out_.movRsiRcx();
            out_.xorEdx();
            const std::string loop = out_.label("cstr.len");
            const std::string done = out_.label("cstr.done");
            out_.def(loop);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, done);
            out_.incRsi();
            out_.incRdx();
            out_.jmp(loop);
            out_.def(done);
            stubCall("__nx_write");
            out_.leaRcxRip("str" + std::to_string(nl));
            out_.movEdxImm32(1);
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_str_eq: rcx = a, rdx = b (NUL-terminated). Returns 0/1 in rax.
            // Implemented with repne/repe string instructions over strlen(a)+1 bytes
            // (the trailing NUL decides prefix cases correctly).
        void Generator::genStrEq() {
            out_.def("__nx_str_eq");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(32);
            out_.movRcxToRbp(-8);
            out_.movRdxToRbp(-16);
            out_.cld();
            out_.movRdiRcx();
            out_.movRcxImmNeg1();
            out_.xorEax();
            out_.repneScasb();
            out_.notRcx();
            out_.decRcx(); // rcx = strlen(a)
            out_.incRcx(); // include the NUL terminator
            out_.movRsiFromRbp(-8);
            out_.movRdiFromRbp(-16);
            out_.repeCmpsb();
            out_.setcc(CC_E);
            out_.movzxEaxAl();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_string_len: rax = NUL-terminated string. Returns byte length in rax.
        void Generator::genStringLen() {
            out_.def("__nx_string_len");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.movRcxRax();
            out_.xorRax();
            const std::string loop = out_.label("strlen.loop");
            const std::string done = out_.label("strlen.done");
            out_.def(loop);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, done);
            out_.incRcx();
            out_.incRax();
            out_.jmp(loop);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_string_byte_at: rax = string, rcx = index. Returns byte (0-255) in rax, or -1 if
            // out of bounds.
        void Generator::genStringByteAt() {
            out_.def("__nx_string_byte_at");
            out_.pushRbp();
            out_.movRbpRsp();
            // rax = string, rcx = index
            // Save string pointer in rdx
            out_.movRdxRax();
            // Check if index < 0 (signed comparison)
            out_.testRcxRcx();
            const std::string neg = out_.label("sba.neg");
            out_.jcc(J_L, neg);
            // Get string length
            out_.movRsiRdx(); // string -> rsi
            out_.xorRax();    // length counter
            const std::string lenLoop = out_.label("sba.len");
            const std::string lenDone = out_.label("sba.lendone");
            out_.def(lenLoop);
            out_.cmpBytePtrRsi(0);
            out_.jcc(J_E, lenDone);
            out_.incRsi();
            out_.incRax();
            out_.jmp(lenLoop);
            out_.def(lenDone);
            // Compare index (rcx) with length (rax)
            out_.cmpRcxRax();
            out_.jcc(J_AE, neg); // index >= length -> out of bounds
            // In bounds: load byte at index (string pointer in rdx)
            out_.movRsiRdx(); // string -> rsi
            out_.addRsiRcx(); // rsi += index
            out_.movAlFromRsi();
            out_.movzxEaxAl();
            const std::string ret = out_.label("sba.ret");
            out_.jmp(ret);
            out_.def(neg);
            out_.movRaxImm64(0xFFFFFFFFFFFFFFFFu); // -1
            out_.def(ret);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

            // __nx_str_cat: rcx = a, rdx = b. Returns a fresh arena string in rax.
        void Generator::genStrCat() {
            out_.def("__nx_str_cat");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.movRcxToRbp(-8);
            out_.movRdxToRbp(-16);
            out_.movRsiFromRbp(-8);
            out_.xorR8d();
            const std::string lenA = out_.label("cat.lenA");
            const std::string gotA = out_.label("cat.gotA");
            const std::string lenB = out_.label("cat.lenB");
            const std::string gotB = out_.label("cat.gotB");
            const std::string oom = out_.label("cat.oom");
            out_.def(lenA);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, gotA);
            out_.incRsi();
            out_.incR8();
            out_.jmp(lenA);
            out_.def(gotA);
            out_.movRsiFromRbp(-16);
            out_.xorR9d();
            out_.def(lenB);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, gotB);
            out_.incRsi();
            out_.incR9();
            out_.jmp(lenB);
            out_.def(gotB);
            out_.movRaxR8();
            out_.addRaxR9();
            out_.incRax();
            out_.leaR10Rip("__nx_arena");
            out_.movR11FromR10Disp8(8);
            out_.movRsiR11();
            out_.addRaxRsi();
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            out_.movR10Disp8FromRax(8);
            out_.movRaxRsi();
            out_.movRdiRsi();
            out_.movRsiFromRbp(-8);
            out_.movRcxR8();
            out_.repMovsb();
            out_.movRsiFromRbp(-16);
            out_.movRcxR9();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

            // __nx_str_i64: rax = value. Returns a fresh arena string in rax.
        void Generator::genStrI64() {
            out_.def("__nx_str_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.leaRsiFromRbp(-32);
            out_.xorR10d();
            out_.testRaxRax();
            const std::string positive = out_.label("s64.pos");
            out_.jcc(J_NS, positive);
            out_.negRax();
            out_.movR10Imm1();
            out_.def(positive);
            out_.movRcxImm10();
            const std::string digit = out_.label("s64.digit");
            const std::string noSign = out_.label("s64.nosign");
            const std::string oom = out_.label("s64.oom");
            out_.def(digit);
            out_.xorEdx();
            out_.divRcx();
            out_.addDl48();
            out_.decRsi();
            out_.storeDlRsi();
            out_.testRaxRax();
            out_.jcc(J_NE, digit);
            out_.testR10d();
            out_.jcc(J_E, noSign);
            out_.decRsi();
            out_.storeByteRsiImm8(0x2D);
            out_.def(noSign);
            out_.leaRdxFromRbp(-32);
            out_.subRdxRsi();
            out_.leaR10Rip("__nx_arena");
            out_.movRdiFromR10Disp8(8);
            out_.movRaxRdx();
            out_.incRax();
            out_.addRaxRdi();
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            out_.movR10Disp8FromRax(8);
            out_.movRaxRdi();
            out_.movRcxRdx();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

            // __nx_input: rax = prompt (NUL-terminated). Prints it, reads one line
            // from stdin, and returns a fresh arena string without the newline.
        void Generator::genInput() {
            out_.def("__nx_input");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRcxRax();
            out_.movRsiRcx();
            out_.xorEdx();
            const std::string plen = out_.label("in.plen");
            const std::string pdone = out_.label("in.pdone");
            out_.def(plen);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, pdone);
            out_.incRsi();
            out_.incRdx();
            out_.jmp(plen);
            out_.def(pdone);
            stubCall("__nx_write");
            // rsi = arena cursor (callee-saved, survives calls); [rbp-8] = start.
            // r10 (arena base) is reloaded at the top of every iteration because
            // OS calls destroy volatile registers.
            out_.leaR10Rip("__nx_arena");
            out_.movR11FromR10Disp8(8);
            out_.movRsiR11();
            out_.movRbpFromRsi(-8);
            if (target_ == NativeTarget::WindowsX64) {
                out_.movEcxImm32(static_cast<std::uint32_t>(-10)); // STD_INPUT_HANDLE
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-16);
            }
            const std::string readLoop = out_.label("in.read");
            const std::string scan = out_.label("in.scan");
            const std::string chunkDone = out_.label("in.chunk");
            const std::string foundNl = out_.label("in.nl");
            const std::string eof = out_.label("in.eof");
            const std::string oom = out_.label("in.oom");
            out_.def(readLoop);
            out_.leaR10Rip("__nx_arena");
            out_.movRaxRsi();
            out_.addRaxImm32(4097);
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            if (target_ == NativeTarget::WindowsX64) {
                out_.movRcxFromRbp(-16);
                out_.movRdxRsi();
                out_.movR8Imm32(4096);
                out_.subRsp(64);
                out_.storeQwordRsp32Imm0();
                out_.leaR9FromRsp8(0x28);
                out_.callIAT("__iat_ReadFile");
                out_.movEaxFromRsp8(0x28);
                out_.addRsp(64);
                out_.testRaxRax();
                out_.jcc(J_E, eof);
                out_.movEcxEax();
            } else {
                out_.movEaxImm32(0); // sys_read
                out_.xorEdi();       // fd 0
                out_.movRdxImm32(4096);
                out_.syscall();
                out_.testRaxRax();
                out_.jcc(J_LE, eof);
                out_.movEcxEax();
            }
            out_.def(scan);
            out_.testEcxEcx();
            out_.jcc(J_E, chunkDone);
            out_.movAlFromRsi();
            out_.cmpAlImm10();
            out_.jcc(J_E, foundNl);
            out_.incRsi();
            out_.decEcx();
            out_.jmp(scan);
            out_.def(chunkDone);
            out_.jmp(readLoop);
            out_.def(foundNl);
            // total = cursor - start; drop a preceding '\r' (Windows console).
            out_.movRaxRsi();
            out_.subRaxFromRbp(-8);
            out_.testRaxRax();
            out_.jcc(J_E, eof);
            out_.movAlFromRsiMinus1();
            out_.cmpAlImm13();
            out_.jcc(J_NE, eof);
            out_.decRsi();
            out_.def(eof);
            out_.storeByteRsiImm0();
            out_.incRsi();
            out_.leaR10Rip("__nx_arena"); // r10 died in the read call
            out_.movR10Disp8FromRsi(8);
            out_.movRaxFromRbp(-8);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

            // __nx_oom: clean out-of-string-memory failure (stderr + exit 1).
        void Generator::genOom() {
            const std::size_t m = internString("NEXUS runtime error: out of string memory\n");
            out_.def("__nx_oom");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genDivZero() {
            const std::size_t m = internString("NEXUS runtime error: division by zero\n");
            out_.def("__nx_divzero");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genDivOverflow() {
            const std::size_t m =
                internString("NEXUS runtime error: signed integer division overflow\n");
            out_.def("__nx_divoverflow");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genEntry() {
            out_.def("_start");
            // Reserve the string bump arena before user code runs: 16 MiB of
            // committed/mapped memory, tracked in the __nx_arena words.
            const std::string noArena = out_.label("start.noarena");
            if (target_ == NativeTarget::WindowsX64) {
                // VirtualAlloc(NULL, 16 MiB, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE).
                // OS entry rsp is 8 (mod 16): reserve 8 bytes padding + 32 shadow.
                out_.xorEcx();
                out_.movRdxImm32(kArenaSize);
                out_.movR8Imm32(0x3000);
                out_.movR9Imm32(0x04);
                out_.subRsp(40);
                out_.callIAT("__iat_VirtualAlloc");
                out_.addRsp(40);
                out_.testRaxRax();
                out_.jcc(J_E, noArena);
            } else {
                // mmap(NULL, 16 MiB, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0).
                out_.xorEdi();
                out_.movRsiImm32(kArenaSize);
                out_.movRdxImm32(3);
                out_.movR10Imm32(0x22);
                out_.movR8Imm32(0xFFFFFFFFu);
                out_.movR9Imm32(0);
                out_.movEaxImm32(9);
                out_.syscall();
                out_.cmpRaxImm8(0xFF); // MAP_FAILED == -1
                out_.jcc(J_E, noArena);
            }
            out_.leaR10Rip("__nx_arena");
            out_.movR10FromRax();
            out_.movR10Disp8FromRax(8);
            out_.call("main");
            if (target_ == NativeTarget::WindowsX64) {
                // ExitProcess(exitCode). The caller's stack is balanced again, so
                // rsp is 8 (mod 16); reserve 8 bytes padding + 32 bytes shadow.
                out_.movEcxEax();
                out_.subRsp(40);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movEdiEax();
                out_.movEaxImm32(60); // sys_exit
                out_.syscall();
            }
            out_.def(noArena);
            out_.jmp("__nx_oom");
        }

    } // namespace detail
} // namespace nexus::native
