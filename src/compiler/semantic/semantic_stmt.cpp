#include "semantic/semantic.hpp"

#include "semantic/semantic_internal.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace nexus {

    bool SemanticAnalyzer::typeMayContainPointer(const Type& type) const {
        std::unordered_set<std::string> visited;
        const auto contains = [&](const auto& self, const Type& candidate) -> bool {
            if (candidate.kind == TypeKind::Pointer)
                return true;
            // A slice is a non-owning view into another value's storage. Treat it
            // as pointer-bearing so the existing borrow-origin analysis also
            // protects slice bindings and function results.
            if (candidate.kind == TypeKind::Slice || candidate.kind == TypeKind::MutableSlice)
                return true;
            if (candidate.kind == TypeKind::Array)
                return candidate.element && self(self, *candidate.element);
            if (candidate.kind == TypeKind::Struct) {
                if (!visited.insert("struct:" + candidate.name).second)
                    return false;
                const auto declaration = structs_.find(candidate.name);
                if (declaration == structs_.end())
                    return false;
                for (const auto& field : declaration->second->fields) {
                    if (self(self, field.type))
                        return true;
                }
            } else if (candidate.kind == TypeKind::Enum) {
                if (!visited.insert("enum:" + candidate.name).second)
                    return false;
                const auto declaration = enums_.find(candidate.name);
                if (declaration == enums_.end())
                    return false;
                for (const auto& variant : declaration->second->variants) {
                    if (variant.payload && self(self, *variant.payload))
                        return true;
                }
            }
            return false;
        };
        return contains(contains, type);
    }

    void SemanticAnalyzer::mergeBorrowState(
        std::unordered_map<std::string, Symbol>& destination,
        const std::unordered_map<std::string, Symbol>& branch) const {
        for (auto& [name, target] : destination) {
            const auto source = branch.find(name);
            if (source == branch.end())
                continue;

            if (target.managedAllocation == source->second.managedAllocation &&
                target.managedAllocationParameter == source->second.managedAllocationParameter) {
                target.managedAllocationFreed =
                    target.managedAllocationFreed || source->second.managedAllocationFreed;
            } else if (target.managedAllocation || source->second.managedAllocation ||
                       target.managedAllocationParameter ||
                       source->second.managedAllocationParameter) {
                // A control-flow join with different allocation identities is
                // intentionally left untracked rather than guessing which
                // allocation the binding contains.
                target.managedAllocation = nullptr;
                target.managedAllocationParameter.reset();
                target.managedAllocationFreed = false;
            }

            if (!typeMayContainPointer(target.type) || !source->second.borrowedFrom)
                continue;
            if (!target.borrowedFrom ||
                source->second.borrowedFrom->scopeDepth > target.borrowedFrom->scopeDepth)
                target.borrowedFrom = source->second.borrowedFrom;
        }
    }

    void SemanticAnalyzer::block(Block& b,
                                 std::unordered_map<std::string, Symbol>& env,
                                 Type ret,
                                 bool inLoop) {
        struct ScopeDepthGuard {
            std::size_t& depth;
            explicit ScopeDepthGuard(std::size_t& value) : depth(value) { ++depth; }
            ~ScopeDepthGuard() { --depth; }
        } scopeDepthGuard(scopeDepth_);

        for (auto& s : b.statements) {
            try {
                if (auto* nested = dynamic_cast<Block*>(s.get())) {
                    auto localEnv = env;
                    block(*nested, localEnv, ret, inLoop);
                    mergeBorrowState(env, localEnv);
                } else if (auto* l = dynamic_cast<LetStmt*>(s.get())) {
                    // `_` is a discard binding: each occurrence gets a fresh hidden
                    // name so repeated `let _ = f()` never collides and `_` stays
                    // unreferenceable.
                    if (l->name == "_")
                        l->name = "_discard" +
                                  std::to_string(++discardCounter_);
                    if (env.contains(l->name))
                        throw std::runtime_error("duplicate variable: " + l->name);
                    normalize(l->type);
                    if (l->type.kind != TypeKind::Unknown) {
                        if (auto* array = dynamic_cast<ArrayExpr*>(l->init.get());
                            array && l->type.kind == TypeKind::Array && l->type.element) {
                            for (auto& item : array->elements)
                                applyNumericLiteralType(item.get(), *l->type.element);
                        } else {
                            applyNumericLiteralType(l->init.get(), l->type);
                        }
                    }
                    auto t = expr(l->init.get(), env);
                    if (containsVoidType(t))
                        throw std::runtime_error("cannot bind a value of type 'void'");
                    if (l->type.kind != TypeKind::Unknown && !sameType(l->type, t))
                        throw std::runtime_error("cannot initialize '" + l->name + "': expected '" +
                                                 describeType(l->type) + "' but got '" +
                                                 describeType(t) + "'");
                    l->type = t;
                    Symbol symbol{t, l->mut, scopeDepth_,
                                  typeMayContainPointer(t) ? pointerOrigin(l->init.get(), env)
                                                           : std::nullopt,
                                  nullptr, std::nullopt, false};
                    symbol.mutableBorrow = t.kind == TypeKind::MutableSlice;
                    if (symbol.mutableBorrow && !symbol.borrowedFrom)
                        throw std::runtime_error("mutable slice must borrow named mutable storage");
                    if (const auto managed = managedAllocationOrigin(l->init.get(), env, !inLoop)) {
                        symbol.managedAllocation = managed->allocation;
                        symbol.managedAllocationParameter = managed->parameter;
                        symbol.managedAllocationFreed = managed->freed;
                    }
                    env[l->name] = std::move(symbol);
                    if (typeContainsSlice(t) && t.kind != TypeKind::Slice &&
                        t.kind != TypeKind::MutableSlice) {
                        diagnostics_.push_back(
                            "nested slice values are not yet supported by code generation" +
                            sourceLocation(l->span));
                    }
                } else if (auto* a = dynamic_cast<AssignStmt*>(s.get())) {
                    auto it = env.find(a->name);
                    if (it == env.end())
                        throw std::runtime_error("unknown variable: " + a->name);
                    if (!it->second.mut)
                        throw std::runtime_error("cannot assign to immutable variable: " + a->name);
                    if (it->second.type.kind == TypeKind::MutableSlice)
                        throw std::runtime_error(
                            "mutable slice bindings cannot be reassigned; create a new borrow in a new scope");
                    applyNumericLiteralType(a->value.get(), it->second.type);
                    auto actual = expr(a->value.get(), env);
                    if (!sameType(actual, it->second.type))
                        throw std::runtime_error("cannot assign to '" + a->name + "': expected '" +
                                                 describeType(it->second.type) + "' but got '" +
                                                 describeType(actual) + "'");
                    const auto origin = typeMayContainPointer(it->second.type)
                                            ? pointerOrigin(a->value.get(), env)
                                            : std::nullopt;
                    if (origin && origin->scopeDepth > it->second.scopeDepth)
                        throw std::runtime_error("cannot store pointer to shorter-lived local '" +
                                                 origin->binding + "' in '" + a->name + "'");
                    it->second.borrowedFrom = origin;
                    const auto managed = managedAllocationOrigin(a->value.get(), env, !inLoop);
                    it->second.managedAllocation = managed ? managed->allocation : nullptr;
                    it->second.managedAllocationParameter = managed ? managed->parameter
                                                                    : std::nullopt;
                    it->second.managedAllocationFreed = managed ? managed->freed : false;
                } else if (auto* da = dynamic_cast<DerefAssignStmt*>(s.get())) {
                    const auto pointerType = expr(da->target.get(), env);
                    if (pointerType.kind != TypeKind::Pointer || !pointerType.pointee)
                        throw std::runtime_error("dereference assignment requires a typed pointer");
                    validateCompoundAssign(da->op, *pointerType.pointee);
                    validateCompoundAssignDivisor(da->op, da->value.get());
                    applyNumericLiteralType(da->value.get(), *pointerType.pointee);
                    const auto actual = expr(da->value.get(), env);
                    if (!sameType(actual, *pointerType.pointee))
                        throw std::runtime_error("pointer assignment expects '" +
                                                 describeType(*pointerType.pointee) + "' but got '" +
                                                 describeType(actual) + "'");
                } else if (auto* ma = dynamic_cast<MemberAssignStmt*>(s.get())) {
                    auto t = expr(ma->object.get(), env);
                    if (t.kind != TypeKind::Struct)
                        throw std::runtime_error("member assignment requires struct");
                    if (const auto* ov = lvalueRootVariable(ma->object.get())) {
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
                    validateCompoundAssignDivisor(ma->op, ma->value.get());
                    applyNumericLiteralType(ma->value.get(), f->type);
                    auto actual = expr(ma->value.get(), env);
                    if (!sameType(actual, f->type))
                        throw std::runtime_error("cannot assign field '" + ma->member +
                                                 "': expected '" + describeType(f->type) +
                                                 "' but got '" + describeType(actual) + "'");
                    if (const auto* root = lvalueRootVariable(ma->object.get())) {
                        auto it = env.find(root->name);
                        const auto origin = typeMayContainPointer(f->type)
                                                ? pointerOrigin(ma->value.get(), env)
                                                : std::nullopt;
                        if (it != env.end() && origin) {
                            if (origin->scopeDepth > it->second.scopeDepth)
                                throw std::runtime_error(
                                    "cannot store pointer to shorter-lived local '" +
                                    origin->binding + "' in '" + root->name + "'");
                            if (!it->second.borrowedFrom ||
                                origin->scopeDepth > it->second.borrowedFrom->scopeDepth)
                                it->second.borrowedFrom = origin;
                        }
                    }
                } else if (auto* ia = dynamic_cast<IndexAssignStmt*>(s.get())) {
                    auto ot = expr(ia->object.get(), env);
                    if (const auto* ov = lvalueRootVariable(ia->object.get())) {
                        auto it = env.find(ov->name);
                        if (it != env.end() && !it->second.mut)
                            throw std::runtime_error(
                                "cannot modify array through immutable variable: " + ov->name);
                    }
                    const bool mutableSlice = ot.kind == TypeKind::MutableSlice;
                    if ((!mutableSlice && ot.kind != TypeKind::Array) || !ot.element)
                        throw std::runtime_error(
                            "index assignment requires a mutable array or mutable slice");
                    if (expr(ia->index.get(), env).kind != TypeKind::I64)
                        throw std::runtime_error("array index must be i64");
                    if (ot.kind == TypeKind::Array &&
                        constantIndexOutOfBounds(ia->index.get(), ot.arraySize))
                        throw std::runtime_error("array index is out of bounds");
                    validateCompoundAssign(ia->op, *ot.element);
                    validateCompoundAssignDivisor(ia->op, ia->value.get());
                    applyNumericLiteralType(ia->value.get(), *ot.element);
                    auto actual = expr(ia->value.get(), env);
                    if (!sameType(actual, *ot.element))
                        throw std::runtime_error("array element assignment expects '" +
                                                 describeType(*ot.element) + "' but got '" +
                                                 describeType(actual) + "'");
                    if (!mutableSlice) {
                    if (const auto* root = lvalueRootVariable(ia->object.get())) {
                        auto it = env.find(root->name);
                        const auto origin = typeMayContainPointer(*ot.element)
                                                ? pointerOrigin(ia->value.get(), env)
                                                : std::nullopt;
                        if (it != env.end() && origin) {
                            if (origin->scopeDepth > it->second.scopeDepth)
                                throw std::runtime_error(
                                    "cannot store pointer to shorter-lived local '" +
                                    origin->binding + "' in '" + root->name + "'");
                            if (!it->second.borrowedFrom ||
                                origin->scopeDepth > it->second.borrowedFrom->scopeDepth)
                                it->second.borrowedFrom = origin;
                        }
                    }
                    }
                } else if (auto* e = dynamic_cast<ExprStmt*>(s.get()))
                    expr(e->expr.get(), env);
                else if (auto* d = dynamic_cast<DeferStmt*>(s.get())) {
                    if (!dynamic_cast<CallExpr*>(d->call.get()))
                        throw std::runtime_error("defer requires a function call");
                    struct DeferredCallGuard {
                        bool& active;
                        bool previous;
                        explicit DeferredCallGuard(bool& value) : active(value), previous(value) {
                            active = true;
                        }
                        ~DeferredCallGuard() { active = previous; }
                    } deferredCallGuard(analyzingDeferredCall_);
                    const auto deferredType = expr(d->call.get(), env);
                    if (deferredType.kind != TypeKind::Void)
                        throw std::runtime_error("defer requires a call returning void");
                } else if (auto* r = dynamic_cast<ReturnStmt*>(s.get())) {
                    if (r->expr)
                        applyNumericLiteralType(r->expr.get(), ret);
                    auto t = r->expr ? expr(r->expr.get(), env) : Type::void_();
                    if (t.kind == TypeKind::MutableSlice)
                        throw std::runtime_error("returning a mutable slice is not supported; pass it to a function instead");
                    if (!sameType(t, ret))
                        throw std::runtime_error("return expects '" + describeType(ret) +
                                                 "' but got '" + describeType(t) + "'");
                    if (ret.kind == TypeKind::I64) {
                        const auto managed =
                            managedAllocationOrigin(r->expr.get(), env, !inLoop);
                        if (!managed) {
                            currentFunctionReturnManagedUnknown_ = true;
                        } else if (managed->parameter) {
                            currentFunctionReturnManagedParams_.push_back(*managed->parameter);
                        } else if (managed->allocation) {
                            currentFunctionReturnManagedFresh_ = true;
                            currentFunctionReturnManagedFreshFreed_ =
                                currentFunctionReturnManagedFreshFreed_ || managed->freed;
                        }
                    }
                    const auto origin = typeMayContainPointer(t) ? pointerOrigin(r->expr.get(), env)
                                                                 : std::nullopt;
                    if (origin && origin->scopeDepth == 0) {
                        const auto parameter = currentFunctionParameterIndices_.find(origin->binding);
                        if (parameter != currentFunctionParameterIndices_.end())
                            currentFunctionReturnBorrowParams_.push_back(parameter->second);
                    }
                    if (origin && origin->scopeDepth > 0)
                        throw std::runtime_error("cannot return a pointer to local storage '" +
                                                 origin->binding + "'");
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
                        // Both branches start from the same incoming state. Merging
                        // one before analyzing the other would create impossible paths.
                        auto c = env;
                        block(*ifs->elseBlock, c, ret, inLoop);
                        mergeBorrowState(env, a);
                        mergeBorrowState(env, c);
                    } else {
                        mergeBorrowState(env, a);
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
                    mergeBorrowState(env, a);
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
                    local[f->name] = {
                        varType, false, scopeDepth_ + 1,
                        typeMayContainPointer(varType)
                            ? pointerOrigin(f->collection.get(), env)
                            : std::nullopt,
                        nullptr,
                        std::nullopt,
                        false};
                    block(*f->body, local, ret, true);
                    mergeBorrowState(env, local);
                } else if (auto* mm = dynamic_cast<MatchStmt*>(s.get())) {

                    auto vt = expr(mm->value.get(), env);
                    if (!isIntegerType(vt.kind) && vt.kind != TypeKind::Bool &&
                        vt.kind != TypeKind::Enum)
                        throw std::runtime_error("match supports integer, bool, or enum values");
                    bool sawCatchAll = false;
                    std::unordered_map<std::string, bool> seenPatterns;
                    std::set<std::string> coveredEnumVariants;
                    std::set<bool> coveredBoolValues;
                    auto mergedArmBorrows = env;
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
                            local[arm.binding] = {
                                bindType, false, scopeDepth_ + 1,
                                typeMayContainPointer(bindType)
                                    ? pointerOrigin(mm->value.get(), env)
                                    : std::nullopt,
                                nullptr,
                                std::nullopt,
                                false};
                        }

                        if (arm.rangeEnd) {
                            if (!isIntegerType(vt.kind))
                                throw std::runtime_error(
                                    "range patterns require an integer value");
                            applyNumericLiteralType(arm.pattern.get(), vt);
                            applyNumericLiteralType(arm.rangeEnd.get(), vt);
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
                            if (isNumericType(vt.kind))
                                applyNumericLiteralType(arm.pattern.get(), vt);
                            auto pt = expr(arm.pattern.get(), env);
                            if (!sameType(pt, vt))
                                throw std::runtime_error("match pattern type mismatch");

                            std::string key;
                            std::string display;
                            if (const auto* integer =
                                    dynamic_cast<const IntExpr*>(arm.pattern.get())) {
                                const auto magnitude = std::to_string(integer->value);
                                display = integer->negative ? "-" + magnitude : magnitude;
                                key = describeType(vt) + ":" + display;
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
                        mergeBorrowState(mergedArmBorrows, local);
                    }
                    mergeBorrowState(env, mergedArmBorrows);
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
                    diagnostic += sourceLocation(s->span);
                diagnostics_.push_back(std::move(diagnostic));
            }
        }
    }

} // namespace nexus
