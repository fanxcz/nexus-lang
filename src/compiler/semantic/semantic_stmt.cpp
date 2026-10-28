#include "semantic/semantic.hpp"

#include "semantic/semantic_internal.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace nexus {

    void SemanticAnalyzer::block(Block& b,
                                 std::unordered_map<std::string, Symbol>& env,
                                 Type ret,
                                 bool inLoop) {

        for (auto& s : b.statements) {
            try {
                if (auto* nested = dynamic_cast<Block*>(s.get())) {
                    auto localEnv = env;
                    block(*nested, localEnv, ret, inLoop);
                } else if (auto* l = dynamic_cast<LetStmt*>(s.get())) {
                    // `_` is a discard binding: each occurrence gets a fresh hidden
                    // name so repeated `let _ = f()` never collides and `_` stays
                    // unreferenceable.
                    if (l->name == "_")
                        l->name = "_discard" +
                                  std::to_string(++discardCounter_);
                    if (env.contains(l->name))
                        throw std::runtime_error("duplicate variable: " + l->name);
                    auto t = expr(l->init.get(), env);
                    normalize(l->type);
                    if (l->type.kind != TypeKind::Unknown && !sameType(l->type, t))
                        throw std::runtime_error("cannot initialize '" + l->name + "': expected '" +
                                                 describeType(l->type) + "' but got '" +
                                                 describeType(t) + "'");
                    l->type = t;
                    env[l->name] = {t, l->mut};
                } else if (auto* a = dynamic_cast<AssignStmt*>(s.get())) {
                    auto it = env.find(a->name);
                    if (it == env.end())
                        throw std::runtime_error("unknown variable: " + a->name);
                    if (!it->second.mut)
                        throw std::runtime_error("cannot assign to immutable variable: " + a->name);
                    auto actual = expr(a->value.get(), env);
                    if (!sameType(actual, it->second.type))
                        throw std::runtime_error("cannot assign to '" + a->name + "': expected '" +
                                                 describeType(it->second.type) + "' but got '" +
                                                 describeType(actual) + "'");
                } else if (auto* da = dynamic_cast<DerefAssignStmt*>(s.get())) {
                    const auto pointerType = expr(da->target.get(), env);
                    if (pointerType.kind != TypeKind::Pointer || !pointerType.pointee)
                        throw std::runtime_error("dereference assignment requires a typed pointer");
                    validateCompoundAssign(da->op, *pointerType.pointee);
                    const auto actual = expr(da->value.get(), env);
                    if (!sameType(actual, *pointerType.pointee))
                        throw std::runtime_error("pointer assignment expects '" +
                                                 describeType(*pointerType.pointee) + "' but got '" +
                                                 describeType(actual) + "'");
                } else if (auto* ma = dynamic_cast<MemberAssignStmt*>(s.get())) {
                    auto t = expr(ma->object.get(), env);
                    if (t.kind != TypeKind::Struct)
                        throw std::runtime_error("member assignment requires struct");
                    if (auto* ov = dynamic_cast<VarExpr*>(ma->object.get())) {
                        auto it = env.find(ov->name);
                        if (it != env.end() && !it->second.mut)
                            throw std::runtime_error(
                                "cannot modify field through immutable variable: " + ov->name);
                    }
                    auto& st = getStruct(t.name);
                    auto* f = fieldOf(st, ma->member);
                    if (!f)
                        throw std::runtime_error("unknown field: " + ma->member);
                    validateCompoundAssign(ma->op, f->type);
                    auto actual = expr(ma->value.get(), env);
                    if (!sameType(actual, f->type))
                        throw std::runtime_error("cannot assign field '" + ma->member +
                                                 "': expected '" + describeType(f->type) +
                                                 "' but got '" + describeType(actual) + "'");
                } else if (auto* ia = dynamic_cast<IndexAssignStmt*>(s.get())) {
                    auto ot = expr(ia->object.get(), env);
                    if (auto* ov = dynamic_cast<VarExpr*>(ia->object.get())) {
                        auto it = env.find(ov->name);
                        if (it != env.end() && !it->second.mut)
                            throw std::runtime_error(
                                "cannot modify array through immutable variable: " + ov->name);
                    }
                    if (ot.kind != TypeKind::Array || !ot.element)
                        throw std::runtime_error("index assignment requires an array");
                    if (expr(ia->index.get(), env).kind != TypeKind::I64)
                        throw std::runtime_error("array index must be i64");
                    if (constantIndexOutOfBounds(ia->index.get(), ot.arraySize))
                        throw std::runtime_error("array index is out of bounds");
                    validateCompoundAssign(ia->op, *ot.element);
                    auto actual = expr(ia->value.get(), env);
                    if (!sameType(actual, *ot.element))
                        throw std::runtime_error("array element assignment expects '" +
                                                 describeType(*ot.element) + "' but got '" +
                                                 describeType(actual) + "'");
                } else if (auto* e = dynamic_cast<ExprStmt*>(s.get()))
                    expr(e->expr.get(), env);
                else if (auto* d = dynamic_cast<DeferStmt*>(s.get())) {
                    if (!dynamic_cast<CallExpr*>(d->call.get()))
                        throw std::runtime_error("defer requires a function call");
                    const auto deferredType = expr(d->call.get(), env);
                    if (deferredType.kind != TypeKind::Void)
                        throw std::runtime_error("defer requires a call returning void");
                } else if (auto* r = dynamic_cast<ReturnStmt*>(s.get())) {
                    auto t = r->expr ? expr(r->expr.get(), env) : Type::void_();
                    if (!sameType(t, ret))
                        throw std::runtime_error("return expects '" + describeType(ret) +
                                                 "' but got '" + describeType(t) + "'");
                } else if (auto* ifs = dynamic_cast<IfStmt*>(s.get())) {
                    {
                        auto actual = expr(ifs->cond.get(), env);
                        if (actual.kind != TypeKind::Bool)
                            throw std::runtime_error("if condition expects 'bool' but got '" +
                                                     describeType(actual) + "'");
                    }
                    auto a = env;
                    block(*ifs->thenBlock, a, ret, inLoop);
                    if (ifs->elseBlock) {
                        auto c = env;
                        block(*ifs->elseBlock, c, ret, inLoop);
                    }
                } else if (auto* w = dynamic_cast<WhileStmt*>(s.get())) {
                    {
                        auto actual = expr(w->cond.get(), env);
                        if (actual.kind != TypeKind::Bool)
                            throw std::runtime_error("while condition expects 'bool' but got '" +
                                                     describeType(actual) + "'");
                    }
                    auto a = env;
                    block(*w->body, a, ret, true);
                } else if (auto* f = dynamic_cast<ForStmt*>(s.get())) {
                    Type varType;
                    if (f->rangeEnd) {
                        auto startType = expr(f->start.get(), env);
                        auto endType = expr(f->rangeEnd.get(), env);
                        if (startType.kind != TypeKind::I64 || endType.kind != TypeKind::I64)
                            throw std::runtime_error(
                                "for range bounds must be i64");
                        varType = Type::i64();
                    } else {
                        auto collectionType = expr(f->collection.get(), env);
                        if (collectionType.kind == TypeKind::String) {
                            // String iteration yields UTF-8 Unicode code points as i64,
                            // matching string.char_at rather than iterating raw bytes.
                            varType = Type::i64();
                        } else if (collectionType.kind == TypeKind::Array &&
                                   collectionType.element) {
                            varType = *collectionType.element;
                        } else {
                            throw std::runtime_error(
                                "for iteration requires an array or string; use "
                                "'for i in start..end' for numeric ranges");
                        }
                    }
                    auto local = env;
                    local[f->name] = {varType, false};
                    block(*f->body, local, ret, true);
                } else if (auto* mm = dynamic_cast<MatchStmt*>(s.get())) {

                    auto vt = expr(mm->value.get(), env);
                    if (vt.kind != TypeKind::I64 && vt.kind != TypeKind::Bool &&
                        vt.kind != TypeKind::Enum)
                        throw std::runtime_error("match supports i64, bool, or enum values");
                    bool sawCatchAll = false;
                    std::unordered_map<std::string, bool> seenPatterns;
                    std::set<std::string> coveredEnumVariants;
                    std::set<bool> coveredBoolValues;
                    for (auto& arm : mm->arms) {
                        const bool catchAll =
                            (arm.wildcard ||
                             (!arm.binding.empty() && !arm.pattern)) &&
                            !arm.guard;
                        if (sawCatchAll)
                            throw std::runtime_error(
                                "unreachable match arm after a catch-all arm");
                        if (catchAll)
                            sawCatchAll = true;

                        auto local = env;
                        if (const auto* ev = dynamic_cast<const EnumVariantExpr*>(arm.pattern.get())) {
                            if (vt.kind != TypeKind::Enum || vt.name != ev->enumName)
                                throw std::runtime_error("enum match pattern '" + ev->enumName +
                                                         "::" + ev->variant + "' does not match value type '" +
                                                         describeType(vt) + "'");
                            const auto& patternEnum = getEnum(ev->enumName);
                            const EnumVariant* patternVariant = nullptr;
                            for (const auto& variant : patternEnum.variants) {
                                if (variant.name == ev->variant) {
                                    patternVariant = &variant;
                                    break;
                                }
                            }
                            if (!patternVariant)
                                throw std::runtime_error("unknown enum variant: " + ev->enumName +
                                                         "::" + ev->variant);
                            if (ev->payload)
                                throw std::runtime_error("enum match payloads must use a binding name, e.g. " +
                                                         ev->enumName + "::" + ev->variant + "(value)");
                            if (!arm.binding.empty() && !patternVariant->payload)
                                throw std::runtime_error("enum variant '" + ev->enumName + "::" +
                                                         ev->variant + "' has no payload to bind");
                            if (arm.binding.empty() && patternVariant->payload)
                                throw std::runtime_error("enum variant '" + ev->enumName + "::" +
                                                         ev->variant + "' requires a payload binding");
                            if (!arm.guard)
                                coveredEnumVariants.insert(ev->variant);
                        }
                        if (!arm.binding.empty()) {
                            if (local.contains(arm.binding))
                                throw std::runtime_error("duplicate variable: " + arm.binding);
                            Type bindType = vt;
                            if (vt.kind == TypeKind::Enum) {
                                if (const auto* ev = dynamic_cast<const EnumVariantExpr*>(arm.pattern.get())) {
                                    const auto& en = getEnum(ev->enumName);
                                    for (const auto& v : en.variants) {
                                        if (v.name == ev->variant && v.payload) {
                                            bindType = *v.payload;
                                            break;
                                        }
                                    }
                                }
                            }
                            local[arm.binding] = {bindType, false};
                        }

                        if (arm.rangeEnd) {
                            if (vt.kind != TypeKind::I64)
                                throw std::runtime_error(
                                    "range patterns require an i64 value");
                            auto startType = expr(arm.pattern.get(), env);
                            auto endType = expr(arm.rangeEnd.get(), env);
                            if (!sameType(startType, vt) || !sameType(endType, vt))
                                throw std::runtime_error(
                                    "match range bounds must have the same type as the value");
                            if (const auto* start =
                                    dynamic_cast<const IntExpr*>(arm.pattern.get())) {
                                if (const auto* end =
                                        dynamic_cast<const IntExpr*>(arm.rangeEnd.get())) {
                                    if (start->value > end->value)
                                        throw std::runtime_error(
                                            "match range lower bound exceeds upper bound");
                                }
                            }
                        } else if (!arm.wildcard && arm.binding.empty()) {
                            auto pt = expr(arm.pattern.get(), env);
                            if (!sameType(pt, vt))
                                throw std::runtime_error("match pattern type mismatch");

                            std::string key;
                            std::string display;
                            if (const auto* integer =
                                    dynamic_cast<const IntExpr*>(arm.pattern.get())) {
                                key = "i64:" + std::to_string(integer->value);
                                display = std::to_string(integer->value);
                            } else if (const auto* boolean =
                                           dynamic_cast<const BoolExpr*>(arm.pattern.get())) {
                                key = std::string("bool:") + (boolean->value ? "true" : "false");
                                display = boolean->value ? "true" : "false";
                                if (!arm.guard)
                                    coveredBoolValues.insert(boolean->value);
                            } else if (const auto* variant = dynamic_cast<const EnumVariantExpr*>(
                                           arm.pattern.get())) {
                                key = "enum:" + variant->enumName + "::" + variant->variant;
                                display = variant->enumName + "::" + variant->variant;
                            }
                            if (!key.empty() && !seenPatterns.emplace(key, true).second)
                                throw std::runtime_error("duplicate match pattern '" + display +
                                                         "'");
                        }

                        if (arm.guard) {
                            auto guardType = expr(arm.guard.get(), local);
                            if (guardType.kind != TypeKind::Bool)
                                throw std::runtime_error("match guard must be bool");
                        }

                        block(*arm.body, local, ret, inLoop);
                    }
                    // Exhaustiveness is only provable for finite domains:
                    // bool (two values) and enums (one value per variant).
                    // An i64 domain cannot be checked, so it keeps the
                    // historical requirement of a catch-all only when the
                    // author wants one.
                    if (!sawCatchAll) {
                        if (vt.kind == TypeKind::Bool) {
                            const bool hasTrue = coveredBoolValues.contains(true);
                            const bool hasFalse = coveredBoolValues.contains(false);
                            if (!hasTrue || !hasFalse)
                                throw std::runtime_error(
                                    "match on bool is not exhaustive: missing '" +
                                    std::string(hasTrue ? "false" : "true") +
                                    "'; add the missing arm or a catch-all");
                        } else if (vt.kind == TypeKind::Enum) {
                            const auto& matchedEnum = getEnum(vt.name);
                            std::string missing;
                            for (const auto& variant : matchedEnum.variants) {
                                if (!coveredEnumVariants.contains(variant.name)) {
                                    if (!missing.empty())
                                        missing += ", ";
                                    missing += vt.name + "::" + variant.name;
                                }
                            }
                            if (!missing.empty())
                                throw std::runtime_error(
                                    "match on enum '" + vt.name +
                                    "' is not exhaustive: missing " + missing +
                                    "; add the missing arms or a catch-all");
                        }
                    }
                } else if (dynamic_cast<BreakStmt*>(s.get()) ||
                           dynamic_cast<ContinueStmt*>(s.get())) {
                    if (!inLoop)
                        throw std::runtime_error("break/continue used outside loop");
                }
            } catch (const std::exception& error) {
                std::string diagnostic = error.what();
                if (diagnostic.rfind(" at ") == std::string::npos && s && s->span.valid)
                    diagnostic += " at " + std::to_string(s->span.start.line) + ":" +
                                  std::to_string(s->span.start.column);
                diagnostics_.push_back(std::move(diagnostic));
            }
        }
    }

} // namespace nexus
