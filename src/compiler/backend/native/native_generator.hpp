#pragma once

#include "native/native.hpp"

#include "lexer/token.hpp"
#include "native/image.hpp"
#include "native/native_internal.hpp"
#include "native/x64_emitter.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace nexus::native {
    namespace detail {

        // Lowers the builtin native v0 subset to x86-64 machine code. The
        // declaration lives here; the implementation is split by topic under
        // native_gen_*.cpp so no single file grows without bound.
        class Generator {
        public:
            Generator(CodeImage& image, NativeTarget target);

            static Kind typeKind(const Type& t);

            void generate(const Program& program);

        private:
            CodeImage& image_;
            Emitter out_;
            NativeTarget target_;
            std::map<std::string, FnSig> sigs_;
            std::map<std::string, std::size_t> slots_;
            std::map<const ForStmt*, std::string> forLimitNames_;
            std::size_t forLimitCounter_ = 0;
            std::size_t slotCount_ = 0;
            std::string endLabel_;
            std::vector<std::string> breakStack_;
            std::vector<std::string> continueStack_;
            const Function* currentFn_ = nullptr;

            static std::int64_t slotOff(std::size_t slot);

            std::size_t internString(const std::string& text);

            void genFunction(const Function& f);

            void collectLocals(const Block* b);

            void genBlock(const Block* b);

            void genStmt(const Stmt* s);

            void genExpr(const Expr* e);

            void genPrint(const Expr* arg);

            // True for string-typed expressions (literals, string vars/params, string
            // concatenation, and the input/str_i64 builtins).
            bool exprIsString(const Expr* e);

            // Mirrors the checker's kind rules to select the right print helper.
            bool exprIsBool(const Expr* e);

            Kind localKind(const std::string& name);

            void findLet(const Block* b, const std::string& name, Kind& found, bool& ok);

            Kind initKind(const Expr* e);

            // Call the shared write helper. r10/r11 do NOT survive OS calls
            // (the syscall instruction itself clobbers rcx+r11, and ntdll
            // trampolines clobber r10), so the stack pointer is sheltered in the
            // callee-saved rbx instead. Nothing else in the stubs uses rbx.
            void stubCall(const std::string& target);

            // __nx_write: rcx = pointer, rdx = length. Writes to stdout.
            void genWrite();

            // __nx_print_i64: rax = value. Prints decimal + newline.
            void genPrintI64();

            // __nx_print_bool: rax = 0/1. Prints "true"/"false" + newline.
            void genPrintBool();

            // __nx_write_cstr: rax = NUL-terminated string. Writes it + newline.
            void genWriteCstr();

            // __nx_str_eq: rcx = a, rdx = b (NUL-terminated). Returns 0/1 in rax.
            // Implemented with repne/repe string instructions over strlen(a)+1 bytes
            // (the trailing NUL decides prefix cases correctly).
            void genStrEq();

            // __nx_string_len: rax = NUL-terminated string. Returns byte length in rax.
            void genStringLen();

            // __nx_string_byte_at: rax = string, rcx = index. Returns byte (0-255) in rax, or -1 if
            // out of bounds.
            void genStringByteAt();

            // __nx_str_cat: rcx = a, rdx = b. Returns a fresh arena string in rax.
            void genStrCat();

            // __nx_str_i64: rax = value. Returns a fresh arena string in rax.
            void genStrI64();

            // __nx_input: rax = prompt (NUL-terminated). Prints it, reads one line
            // from stdin, and returns a fresh arena string without the newline.
            void genInput();

            // __nx_oom: clean out-of-string-memory failure (stderr + exit 1).
            void genOom();

            void genEntry();
        };

    } // namespace detail
} // namespace nexus::native
