#include "native/native_generator.hpp"

#include <stdexcept>

namespace nexus::native {
    namespace detail {

        Generator::Generator(CodeImage& image, NativeTarget target)
            : image_(image), out_(image), target_(target) {}

        Kind Generator::typeKind(const Type& t) {
            if (t.kind == TypeKind::Bool)
                return Kind::Bool;
            if (t.kind == TypeKind::String)
                return Kind::Str;
            if (t.kind == TypeKind::Void)
                return Kind::Void;
            return Kind::I64;
        }

        void Generator::generate(const Program& program) {
            for (const auto& f : program.functions) {
                FnSig sig;
                for (const auto& p : f.params)
                    sig.params.push_back(typeKind(p.type));
                sig.ret = typeKind(f.ret);
                sigs_[f.name] = sig;
            }
            for (const auto& f : program.functions)
                genFunction(f);
            genWrite();
            genWriteCstr();
            genStrEq();
            genStringLen();
            genStringByteAt();
            genStrCat();
            genStrI64();
            genInput();
            genOom();
            genDivZero();
            genDivOverflow();
            genPrintI64();
            genPrintBool();
            genEntry();
        }

        std::int64_t Generator::slotOff(std::size_t slot) {
            return -static_cast<std::int64_t>(8 * (slot + 1));
        }

        std::size_t Generator::internString(const std::string& text) {
            for (std::size_t i = 0; i < image_.strings.size(); ++i)
                if (image_.strings[i] == text)
                    return i;
            image_.strings.push_back(text);
            return image_.strings.size() - 1;
        }

        void Generator::genFunction(const Function& f) {
            currentFn_ = &f;
            slots_.clear();
            forLimitNames_.clear();
            forLimitCounter_ = 0;
            slotCount_ = 0;
            for (const auto& p : f.params)
                slots_[p.name] = slotCount_++;
            // Pre-scan let bindings so every local owns a fixed frame slot.
            collectLocals(f.body.get());
            const std::size_t frame = (slotCount_ * 8 + 15) / 16 * 16;

            out_.def(f.name);
            out_.pushRbp();
            out_.movRbpRsp();
            if (frame > 0)
                out_.subRsp(static_cast<std::uint32_t>(frame));
            // Arguments arrive on the stack, pushed left-to-right, so argument k
            // sits at rbp+16+8*(nargs-1-k) once the frame is established.
            const std::size_t nargs = f.params.size();
            for (std::size_t k = 0; k < nargs; ++k) {
                const std::int64_t src = 16 + static_cast<std::int64_t>(8 * (nargs - 1 - k));
                out_.movR11FromRbp(src);
                out_.movRbpFromR11(slotOff(slots_[f.params[k].name]));
            }
            endLabel_ = out_.label(f.name + "$end");
            breakStack_.clear();
            continueStack_.clear();
            genBlock(f.body.get());
            out_.def(endLabel_);
            if (f.name == "main" && f.ret.kind == TypeKind::Void)
                out_.xorEax();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            currentFn_ = nullptr;
        }

        void Generator::collectLocals(const Block* b) {
            for (const auto& s : b->statements) {
                if (const auto* nested = dynamic_cast<const Block*>(s.get())) {
                    collectLocals(nested);
                } else if (const auto* l = dynamic_cast<const LetStmt*>(s.get())) {
                    if (!slots_.contains(l->name))
                        slots_[l->name] = slotCount_++;
                } else if (const auto* i = dynamic_cast<const IfStmt*>(s.get())) {
                    collectLocals(i->thenBlock.get());
                    if (i->elseBlock)
                        collectLocals(i->elseBlock.get());
                } else if (const auto* w = dynamic_cast<const WhileStmt*>(s.get())) {
                    collectLocals(w->body.get());
                } else if (const auto* f = dynamic_cast<const ForStmt*>(s.get())) {
                    if (f->rangeEnd) {
                        if (!slots_.contains(f->name))
                            slots_[f->name] = slotCount_++;
                        const std::string limitName =
                            std::string("\x01native_for_limit_") +
                            std::to_string(forLimitCounter_++);
                        forLimitNames_[f] = limitName;
                        slots_[limitName] = slotCount_++;
                    }
                    collectLocals(f->body.get());
                }
            }
        }

        void Generator::genBlock(const Block* b) {
            for (const auto& s : b->statements)
                genStmt(s.get());
        }

    } // namespace detail
} // namespace nexus::native
