#include "codegen/llvm_cg.hpp"

#include <stdexcept>

namespace nexus {


    bool CG::matchExhaustive(const MatchStmt* match, const Type& type) const {
        bool coversTrue = false;
        bool coversFalse = false;
        std::unordered_map<std::string, bool> coveredVariants;

        for (const auto& arm : match->arms) {
            // Guarded patterns may fail, so they cannot prove domain coverage.
            if (arm.guard)
                continue;

            if (arm.wildcard || (!arm.binding.empty() && !arm.pattern))
                return true;

            if (type.kind == TypeKind::Bool) {
                if (const auto* value = dynamic_cast<const BoolExpr*>(arm.pattern.get())) {
                    if (value->value)
                        coversTrue = true;
                    else
                        coversFalse = true;
                }
            } else if (type.kind == TypeKind::Enum) {
                if (const auto* variant =
                        dynamic_cast<const EnumVariantExpr*>(arm.pattern.get())) {
                    if (variant->enumName == type.name)
                        coveredVariants.emplace(variant->variant, true);
                }
            }
        }

        if (type.kind == TypeKind::Bool)
            return coversTrue && coversFalse;

        if (type.kind == TypeKind::Enum) {
            const auto& declaration = getEnum(type.name);
            if (declaration.variants.empty())
                return false;
            for (const auto& variant : declaration.variants) {
                if (!coveredVariants.contains(variant.name))
                    return false;
            }
            return true;
        }
        return false;
    }

    bool CG::stmtBlock(const Block* nested, Type ret) {
        auto savedVars = vars;
        auto savedTypes = types;
        deferredCalls.emplace_back();
        for (const auto& x : nested->statements) {
            stmt(x.get(), ret);
            if (terminated)
                break;
        }
        const bool blockTerminated = terminated;
        if (!blockTerminated)
            emitDefersFrom(deferredCalls.size() - 1);
        deferredCalls.pop_back();
        vars = std::move(savedVars);
        types = std::move(savedTypes);
        return blockTerminated;
    }

    bool CG::stmt(const Stmt* s, Type ret) {

        if (terminated)
            return true;

        if (auto* nested = dynamic_cast<const Block*>(s))
            return stmtBlock(nested, ret);
        if (auto* l = dynamic_cast<const LetStmt*>(s)) {
            auto ptr = next();
            vars[l->name] = ptr;
            types[l->name] = l->type;
            emit(ptr + " = alloca " + llvmType(l->type));
            auto v = expr(l->init.get());
            emit("store " + llvmType(l->type) + " " + v + ", ptr " + ptr);
            return false;
        }
        if (auto* a = dynamic_cast<const AssignStmt*>(s)) {
            auto v = expr(a->value.get());
            emit("store " + llvmType(types.at(a->name)) + " " + v + ", ptr " +
                 vars.at(a->name));
            return false;
        }
        if (auto* a = dynamic_cast<const MemberAssignStmt*>(s)) {
            auto ot = typeOf(a->object.get());
            auto base = addressOf(a->object.get());
            const auto& st = getStruct(ot.name);
            auto idx = fieldIndex(st, a->member);
            auto ftype = st.fields[idx].type;
            auto addr = next();
            emit(addr + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                 ", i32 0, i32 " + std::to_string(idx));
            if (a->op == TokenKind::Equal) {
                auto v = expr(a->value.get());
                emit("store " + llvmType(ftype) + " " + v + ", ptr " + addr);
                return false;
            }
            auto current = next();
            emit(current + " = load " + llvmType(ftype) + ", ptr " + addr);
            auto v = expr(a->value.get());
            auto combined = compoundValue(a->op, ftype, current, v);
            emit("store " + llvmType(ftype) + " " + combined + ", ptr " + addr);
            return false;
        }
        if (auto* a = dynamic_cast<const IndexAssignStmt*>(s)) {
            Type ot = typeOf(a->object.get());
            if (ot.kind != TypeKind::Array || !ot.element)
                throw std::runtime_error("index assignment requires array");
            auto idx = expr(a->index.get());
            emit("call void @nexus_bounds_check(i64 " + idx + ", i64 " +
                 std::to_string(ot.arraySize) + ")");
            auto base = addressOf(a->object.get());
            auto addr = next();
            emit(addr + " = getelementptr inbounds " + llvmType(ot) + ", ptr " + base +
                 ", i64 0, i64 " + idx);
            if (a->op == TokenKind::Equal) {
                auto v = expr(a->value.get());
                emit("store " + llvmType(*ot.element) + " " + v + ", ptr " + addr);
                return false;
            }
            auto current = next();
            emit(current + " = load " + llvmType(*ot.element) + ", ptr " + addr);
            auto v = expr(a->value.get());
            auto combined = compoundValue(a->op, *ot.element, current, v);
            emit("store " + llvmType(*ot.element) + " " + combined + ", ptr " + addr);
            return false;
        }
        if (auto* a = dynamic_cast<const DerefAssignStmt*>(s)) {
            const Type pointerType = typeOf(a->target.get());
            if (pointerType.kind != TypeKind::Pointer || !pointerType.pointee)
                throw std::runtime_error("dereference assignment requires a typed pointer");
            auto address = expr(a->target.get());
            if (a->op == TokenKind::Equal) {
                auto value = expr(a->value.get());
                emit("store " + llvmType(*pointerType.pointee) + " " + value +
                     ", ptr " + address);
                return false;
            }
            auto current = next();
            emit(current + " = load " + llvmType(*pointerType.pointee) + ", ptr " + address);
            auto value = expr(a->value.get());
            auto combined = compoundValue(a->op, *pointerType.pointee, current, value);
            emit("store " + llvmType(*pointerType.pointee) + " " + combined +
                 ", ptr " + address);
            return false;
        }
        if (dynamic_cast<const ExprStmt*>(s)) {
            expr(static_cast<const ExprStmt*>(s)->expr.get());
            return false;
        }
        if (auto* d = dynamic_cast<const DeferStmt*>(s)) {
            if (deferredCalls.empty())
                throw std::runtime_error("defer used outside a lexical scope");
            const auto* call = dynamic_cast<const CallExpr*>(d->call.get());
            deferredCalls.back().push_back(deferredCall(call));
            return false;
        }
        if (auto* r = dynamic_cast<const ReturnStmt*>(s)) {
            // Evaluate the return value before running cleanup calls, but emit
            // the actual return only after every active scope has been cleaned.
            std::string value;
            if (r->expr)
                value = expr(r->expr.get());
            emitDefersFrom(0);
            if (r->expr) {
                emit("ret " + llvmType(ret) + " " + value);
            } else if (ret.kind == TypeKind::Void) {
                emit("ret void");
            } else {
                emit("ret " + llvmType(ret) + " 0");
            }
            terminated = true;
            return true;
        }
        if (auto* i = dynamic_cast<const IfStmt*>(s)) {
            auto c = expr(i->cond.get()), lt = lab("if.then"), lf = lab("if.else"),
                 le = lab("if.end");
            emit("br i1 " + c + ", label %" + lt + ", label %" + lf);
            startBlock(lt);
            terminated = false;
            const bool tt = stmtBlock(i->thenBlock.get(), ret);
            if (!tt)
                emit("br label %" + le);
            startBlock(lf);
            terminated = false;
            bool et = false;
            if (i->elseBlock) {
                et = stmtBlock(i->elseBlock.get(), ret);
            }
            if (!et)
                emit("br label %" + le);
            if (tt && et) {
                terminated = true;
                return true;
            }
            startBlock(le);
            terminated = false;
            return false;
        }
        if (auto* w = dynamic_cast<const WhileStmt*>(s)) {
            auto lc = lab("while.cond"), lb = lab("while.body"), le = lab("while.end");
            emit("br label %" + lc);
            startBlock(lc);
            auto c = expr(w->cond.get());
            emit("br i1 " + c + ", label %" + lb + ", label %" + le);
            startBlock(lb);
            terminated = false;
            breakLabels.push_back(le);
            continueLabels.push_back(lc);
            loopDeferScopeDepth.push_back(deferredCalls.size());
            stmtBlock(w->body.get(), ret);
            loopDeferScopeDepth.pop_back();
            breakLabels.pop_back();
            continueLabels.pop_back();
            if (!terminated)
                emit("br label %" + lc);
            startBlock(le);
            terminated = false;
            return false;
        }
        if (auto* f = dynamic_cast<const ForStmt*>(s)) {
            auto lc = lab("for.cond"), lb = lab("for.body"), li = lab("for.inc"),
                 le = lab("for.end");
            const auto savedVars = vars;
            const auto savedTypes = types;
            Type varType;

            if (f->rangeEnd) {
                varType = Type::i64();
                auto varPtr = next();
                vars[f->name] = varPtr;
                types[f->name] = varType;
                emit(varPtr + " = alloca i64");
                auto startValue = expr(f->start.get());
                emit("store i64 " + startValue + ", ptr " + varPtr);

                // The range end is evaluated once, before the loop starts.
                auto endPtr = next();
                emit(endPtr + " = alloca i64");
                auto endValue = expr(f->rangeEnd.get());
                emit("store i64 " + endValue + ", ptr " + endPtr);

                emit("br label %" + lc);
                startBlock(lc);
                auto current = next();
                emit(current + " = load i64, ptr " + varPtr);
                auto limit = next();
                emit(limit + " = load i64, ptr " + endPtr);
                auto inRange = next();
                emit(inRange + " = icmp slt i64 " + current + ", " + limit);
                emit("br i1 " + inRange + ", label %" + lb + ", label %" + le);
            } else {
                auto collectionType = typeOf(f->collection.get());
                const bool stringCollection = collectionType.kind == TypeKind::String;
                varType = stringCollection ? Type::i64() : *collectionType.element;
                auto collectionPtr = next();
                emit(collectionPtr + " = alloca " + llvmType(collectionType));
                auto collectionValue = expr(f->collection.get());
                emit("store " + llvmType(collectionType) + " " + collectionValue + ", ptr " +
                     collectionPtr);

                // Arrays use their compile-time length; strings use Unicode code-point count.
                auto lengthPtr = next();
                emit(lengthPtr + " = alloca i64");
                if (stringCollection) {
                    auto length = next();
                    emit(length + " = call i64 @nexus_string_char_count(ptr " + collectionValue + ")");
                    emit("store i64 " + length + ", ptr " + lengthPtr);
                } else {
                    emit("store i64 " + std::to_string(collectionType.arraySize) + ", ptr " +
                         lengthPtr);
                }

                auto indexPtr = next();
                emit(indexPtr + " = alloca i64");
                emit("store i64 0, ptr " + indexPtr);
                auto varPtr = next();
                vars[f->name] = varPtr;
                types[f->name] = varType;
                emit(varPtr + " = alloca " + llvmType(varType));

                emit("br label %" + lc);
                startBlock(lc);
                auto current = next();
                emit(current + " = load i64, ptr " + indexPtr);
                auto length = next();
                emit(length + " = load i64, ptr " + lengthPtr);
                auto inRange = next();
                emit(inRange + " = icmp slt i64 " + current + ", " + length);
                emit("br i1 " + inRange + ", label %" + lb + ", label %" + le);
                startBlock(lb);
                if (stringCollection) {
                    auto element = next();
                    emit(element + " = call i64 @nexus_string_char_at(ptr " + collectionValue +
                         ", i64 " + current + ")");
                    emit("store i64 " + element + ", ptr " + varPtr);
                } else {
                    auto elementPtr = next();
                    emit(elementPtr + " = getelementptr inbounds " + llvmType(collectionType) +
                         ", ptr " + collectionPtr + ", i64 0, i64 " + current);
                    auto element = next();
                    emit(element + " = load " + llvmType(varType) + ", ptr " + elementPtr);
                    emit("store " + llvmType(varType) + " " + element + ", ptr " + varPtr);
                }
                terminated = false;
                breakLabels.push_back(le);
                continueLabels.push_back(li);
                loopDeferScopeDepth.push_back(deferredCalls.size());
                stmtBlock(f->body.get(), ret);
                loopDeferScopeDepth.pop_back();
                breakLabels.pop_back();
                continueLabels.pop_back();
                if (!terminated)
                    emit("br label %" + li);
                startBlock(li);
                terminated = false;
                auto index = next();
                emit(index + " = load i64, ptr " + indexPtr);
                auto nextIndex = next();
                emit(nextIndex + " = add i64 " + index + ", 1");
                emit("store i64 " + nextIndex + ", ptr " + indexPtr);
                emit("br label %" + lc);
            }

            if (f->rangeEnd) {
                startBlock(lb);
                terminated = false;
                breakLabels.push_back(le);
                continueLabels.push_back(li);
                loopDeferScopeDepth.push_back(deferredCalls.size());
                stmtBlock(f->body.get(), ret);
                loopDeferScopeDepth.pop_back();
                breakLabels.pop_back();
                continueLabels.pop_back();
                if (!terminated)
                    emit("br label %" + li);
                startBlock(li);
                terminated = false;
                auto current = next();
                emit(current + " = load i64, ptr " + vars[f->name]);
                auto nextValue = next();
                emit(nextValue + " = add i64 " + current + ", 1");
                emit("store i64 " + nextValue + ", ptr " + vars[f->name]);
                emit("br label %" + lc);
            }

            vars = savedVars;
            types = savedTypes;
            startBlock(le);
            terminated = false;
            return false;
        }
        if (auto* mm = dynamic_cast<const MatchStmt*>(s)) {

            auto value = expr(mm->value.get());
            std::string valueTag = value;
            Type vt = typeOf(mm->value.get());
            if (vt.kind == TypeKind::Enum) {
                auto t = next();
                emit(t + " = extractvalue { i64, i64 } " + value + ", 0");
                valueTag = t;
            }
            auto end = lab("match.end");
            std::vector<std::string> bodies, tests;
            for (std::size_t k = 0; k < mm->arms.size(); ++k) {
                bodies.push_back(lab("match.body"));
                tests.push_back(k + 1 < mm->arms.size() ? lab("match.test") : end);
            }
            std::string first = lab("match.test");
            emit("br label %" + first);
            std::string current = first;
            bool allTerminated = true;
            bool hasCatchAll = false;
            for (std::size_t k = 0; k < mm->arms.size(); ++k) {
                startBlock(current);
                auto& arm = mm->arms[k];
                const auto savedVars = vars;
                const auto savedTypes = types;
                if (!arm.binding.empty()) {
                    Type bindType = vt;
                    std::string bindValue = value;
                    if (vt.kind == TypeKind::Enum) {
                        const auto* evp = dynamic_cast<const EnumVariantExpr*>(arm.pattern.get());
                        if (evp) {
                            const auto& en = getEnum(evp->enumName);
                            for (const auto& v : en.variants) {
                                if (v.name == evp->variant && v.payload) {
                                    bindType = *v.payload;
                                    auto p = next();
                                    emit(p + " = extractvalue { i64, i64 } " + value + ", 1");
                                    if (bindType.kind == TypeKind::String || bindType.kind == TypeKind::Pointer) {
                                        auto c = next();
                                        emit(c + " = inttoptr i64 " + p + " to ptr");
                                        bindValue = c;
                                    } else if (bindType.kind == TypeKind::Bool) {
                                        auto c = next();
                                        emit(c + " = trunc i64 " + p + " to i1");
                                        bindValue = c;
                                    } else if (bindType.kind == TypeKind::F64) {
                                        auto c = next();
                                        emit(c + " = bitcast i64 " + p + " to double");
                                        bindValue = c;
                                    } else {
                                        bindValue = p;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    auto ptr = next();
                    vars[arm.binding] = ptr;
                    types[arm.binding] = bindType;
                    emit(ptr + " = alloca " + llvmType(bindType));
                    emit("store " + llvmType(bindType) + " " + bindValue + ", ptr " + ptr);
                }
                const std::string matched =
                    arm.guard ? lab("match.guard") : bodies[k];
                if (arm.wildcard || (!arm.binding.empty() && !arm.pattern)) {
                    emit("br label %" + matched);
                } else {
                    std::string pat;
                    if (vt.kind == TypeKind::Enum) {
                        if (const auto* enumPattern = dynamic_cast<const EnumVariantExpr*>(arm.pattern.get()))
                            pat = std::to_string(enumTag(*enumPattern));
                        else
                            pat = expr(arm.pattern.get());
                    } else {
                        pat = expr(arm.pattern.get());
                    }
                    if (arm.rangeEnd) {
                        auto upper = expr(arm.rangeEnd.get());
                        auto lowerOk = next();
                        auto upperOk = next();
                        emit(lowerOk + " = icmp sge i64 " + valueTag + ", " + pat);
                        emit(upperOk + " = icmp sle i64 " + valueTag + ", " + upper);
                        auto cmp = next();
                        emit(cmp + " = and i1 " + lowerOk + ", " + upperOk);
                        emit("br i1 " + cmp + ", label %" + matched + ", label %" +
                             tests[k]);
                    } else {
                        auto cmp = next();
                        if (vt.kind == TypeKind::Bool)
                            emit(cmp + " = icmp eq i1 " + value + ", " + pat);
                        else
                            emit(cmp + " = icmp eq i64 " + valueTag + ", " + pat);
                        emit("br i1 " + cmp + ", label %" + matched + ", label %" +
                             tests[k]);
                    }
                }
                if (arm.guard) {
                    startBlock(matched);
                    auto guard = expr(arm.guard.get());
                    emit("br i1 " + guard + ", label %" + bodies[k] + ", label %" +
                         tests[k]);
                }
                startBlock(bodies[k]);
                terminated = false;
                stmtBlock(arm.body.get(), ret);
                if (!terminated) {
                    allTerminated = false;
                    emit("br label %" + end);
                }
                if ((arm.wildcard || (!arm.binding.empty() && !arm.pattern)) && !arm.guard)
                    hasCatchAll = true;
                terminated = false;
                vars = savedVars;
                types = savedTypes;
                if (k + 1 < mm->arms.size())
                    current = tests[k];
            }
            const bool exhaustive = hasCatchAll || matchExhaustive(mm, vt);
            if (exhaustive && allTerminated) {
                // A finite-domain match with no wildcard still has a syntactic
                // final failure edge. It is unreachable after semantic checking;
                // terminate that basic block explicitly so the LLVM CFG is valid.
                if (!hasCatchAll) {
                    startBlock(end);
                    emit("unreachable");
                }
                terminated = true;
                return true;
            }
            startBlock(end);
            terminated = false;
            return false;
        }
        if (dynamic_cast<const BreakStmt*>(s)) {
            if (breakLabels.empty() || loopDeferScopeDepth.empty())
                throw std::runtime_error("break outside loop");
            emitDefersFrom(loopDeferScopeDepth.back());
            emit("br label %" + breakLabels.back());
            terminated = true;
            return true;
        }
        if (dynamic_cast<const ContinueStmt*>(s)) {
            if (continueLabels.empty() || loopDeferScopeDepth.empty())
                throw std::runtime_error("continue outside loop");
            emitDefersFrom(loopDeferScopeDepth.back());
            emit("br label %" + continueLabels.back());
            terminated = true;
            return true;
        }
        return false;
    }

} // namespace nexus
