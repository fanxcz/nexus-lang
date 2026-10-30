#include "semantic/semantic.hpp"

#include "builtins.hpp"
#include "semantic/semantic_internal.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace nexus {

    const StructDecl& SemanticAnalyzer::getStruct(const std::string& n) const {
        auto it = structs_.find(n);
        if (it == structs_.end())
            throw std::runtime_error("unknown struct: " + n);
        return *it->second;
    }
    const EnumDecl& SemanticAnalyzer::getEnum(const std::string& n) const {
        auto it = enums_.find(n);
        if (it == enums_.end())
            throw std::runtime_error("unknown enum: " + n);
        return *it->second;
    }
    // Return-path analysis follows the finite-domain coverage rules used by
    // semantic match validation. Guards cannot prove coverage because they may fail.
    bool SemanticAnalyzer::blockAlwaysReturns(const Block& block) const {
        for (const auto& statement : block.statements) {
            if (dynamic_cast<const ReturnStmt*>(statement.get()))
                return true;

            if (const auto* nested = dynamic_cast<const Block*>(statement.get())) {
                if (blockAlwaysReturns(*nested))
                    return true;
            }

            if (const auto* branch = dynamic_cast<const IfStmt*>(statement.get())) {
                if (branch->elseBlock && blockAlwaysReturns(*branch->thenBlock) &&
                    blockAlwaysReturns(*branch->elseBlock)) {
                    return true;
                }
            }

            if (const auto* match = dynamic_cast<const MatchStmt*>(statement.get())) {
                if (match->arms.empty())
                    continue;

                bool catchAll = false;
                bool everyArmReturns = true;
                bool coversTrue = false;
                bool coversFalse = false;
                std::string enumName;
                std::unordered_map<std::string, bool> coveredVariants;

                for (const auto& arm : match->arms) {
                    if (!arm.body || !blockAlwaysReturns(*arm.body))
                        everyArmReturns = false;

                    if (const auto* variant =
                            dynamic_cast<const EnumVariantExpr*>(arm.pattern.get())) {
                        if (enumName.empty())
                            enumName = variant->enumName;
                        if (!arm.guard && variant->enumName == enumName)
                            coveredVariants.emplace(variant->variant, true);
                    }

                    if (arm.guard)
                        continue;

                    if (arm.wildcard || (!arm.binding.empty() && !arm.pattern))
                        catchAll = true;

                    if (const auto* boolean = dynamic_cast<const BoolExpr*>(arm.pattern.get())) {
                        if (boolean->value)
                            coversTrue = true;
                        else
                            coversFalse = true;
                    }
                }

                if (!everyArmReturns)
                    continue;
                if (catchAll || (coversTrue && coversFalse))
                    return true;

                if (!enumName.empty()) {
                    const auto found = enums_.find(enumName);
                    if (found != enums_.end() && !found->second->variants.empty()) {
                        bool exhaustive = true;
                        for (const auto& variant : found->second->variants) {
                            if (!coveredVariants.contains(variant.name)) {
                                exhaustive = false;
                                break;
                            }
                        }
                        if (exhaustive)
                            return true;
                    }
                }
            }
        }
        return false;
    }

    void SemanticAnalyzer::normalize(Type& type) const {
        if ((type.kind == TypeKind::Struct || type.kind == TypeKind::Enum) &&
            enums_.contains(type.name)) {
            type.kind = TypeKind::Enum;
        }
        if (type.kind == TypeKind::Pointer && type.pointee)
            normalize(*type.pointee);
        if (type.kind == TypeKind::Array && type.element)
            normalize(*type.element);
    }
    void SemanticAnalyzer::analyze(Program& p) {

        diagnostics_.clear();
        currentFunctionReturnType_ = Type::void_();
        scopeDepth_ = 0;
        returnedBorrowParams_.clear();
        currentFunctionParameterIndices_.clear();
        currentFunctionReturnBorrowParams_.clear();
        returnedManagedParams_.clear();
        returnedManagedFresh_.clear();
        returnedManagedUnknown_.clear();
        freedManagedParams_.clear();
        currentFunctionReturnManagedParams_.clear();
        currentFunctionFreedManagedParams_.clear();
        currentFunctionReturnManagedFresh_ = false;
        currentFunctionReturnManagedUnknown_ = false;
        funcs_.clear();
        structs_.clear();
        enums_.clear();

        const auto located = [](const std::string& message, const SourceSpan& span) {
            if (!span.valid || message.rfind(" at ") != std::string::npos)
                return message;
            return message + sourceLocation(span);
        };
        const auto report = [&](const std::string& message, const SourceSpan& span) {
            diagnostics_.push_back(located(message, span));
        };

        for (auto& s : p.structs) {
            if (structs_.contains(s.name) || enums_.contains(s.name)) {
                report("duplicate type: " + s.name, s.span);
            } else {
                structs_[s.name] = &s;
            }

            std::unordered_map<std::string, bool> fieldNames;
            for (const auto& field : s.fields) {
                if (fieldNames.contains(field.name)) {
                    report("duplicate field '" + field.name + "' in struct '" + s.name + "'",
                           field.span.valid ? field.span : s.span);
                } else {
                    fieldNames[field.name] = true;
                }
            }
        }
        for (auto& e : p.enums) {
            if (structs_.contains(e.name) || enums_.contains(e.name)) {
                report("duplicate type: " + e.name, e.span);
            } else {
                enums_[e.name] = &e;
            }
            std::unordered_map<std::string, bool> variantNames;
            for (const auto& variant : e.variants) {
                if (variantNames.contains(variant.name))
                    report("duplicate enum variant '" + variant.name + "' in enum '" + e.name + "'",
                           e.span);
                else
                    variantNames[variant.name] = true;
            }
        }
        for (auto& f : p.functions) {
            if (funcs_.contains(f.name)) {
                report("duplicate function: " + f.name, f.span);
                continue;
            }
            if (builtinSignature(f.name).has_value()) {
                report("function name is reserved by builtin runtime: " + f.name, f.span);
                continue;
            }
            funcs_[f.name] = &f;
        }
        std::function<void(const Type&, bool)> validateType;

        validateType = [&](const Type& t, bool allowVoid) {
            if (t.kind == TypeKind::Void && !allowVoid)
                throw std::runtime_error("'void' is only valid as a function return type");

            if (t.kind == TypeKind::Struct && !structs_.contains(t.name))
                throw std::runtime_error("unknown type: " + t.name);

            if (t.kind == TypeKind::Enum && !enums_.contains(t.name))
                throw std::runtime_error("unknown enum: " + t.name);

            if (t.kind == TypeKind::Pointer && t.pointee) {
                validateType(*t.pointee, false);
            }
            if (t.kind == TypeKind::Array && t.element) {
                validateType(*t.element, false);
            }
        };

        const auto validateTypeAt = [&](const Type& type, const SourceSpan& span,
                                        bool allowVoid) {
            try {
                validateType(type, allowVoid);
            } catch (const std::exception& error) {
                report(error.what(), span);
            }
        };

        for (auto& e : p.enums) {
            for (auto& variant : e.variants) {
                if (!variant.payload)
                    continue;
                normalize(*variant.payload);
                validateTypeAt(*variant.payload, variant.span.valid ? variant.span : e.span, false);
                const auto kind = variant.payload->kind;
                if (kind != TypeKind::I64 && kind != TypeKind::F64 &&
                    kind != TypeKind::Bool && kind != TypeKind::String &&
                    kind != TypeKind::Pointer) {
                    report("enum payloads currently support i64, f64, bool, string, or pointer types; got '" +
                               describeType(*variant.payload) + "'",
                           variant.span.valid ? variant.span : e.span);
                }
            }
        }

        for (auto& s : p.structs)
            for (auto& field : s.fields) {
                normalize(field.type);
                validateTypeAt(field.type, field.span.valid ? field.span : s.span, false);
            }

        // Normalize every signature before deriving interprocedural borrow summaries.
        for (auto& f : p.functions) {
            for (auto& a : f.params) {
                normalize(a.type);
                validateTypeAt(a.type, a.span.valid ? a.span : f.span, false);
            }
            normalize(f.ret);
            validateTypeAt(f.ret, f.span, true);
        }

        // Infer a conservative set of parameters that can flow into a function result.
        // Run to a fixed point so forward-declared helper functions work as well.
        for (std::size_t pass = 0; pass <= p.functions.size(); ++pass) {
            bool changed = false;
            for (auto& f : p.functions) {
                if (f.external || !f.body || !typeMayContainPointer(f.ret))
                    continue;

                std::unordered_map<std::string, Symbol> env;
                currentFunctionParameterIndices_.clear();
                currentFunctionReturnBorrowParams_.clear();
                for (std::size_t index = 0; index < f.params.size(); ++index) {
                    const auto& parameter = f.params[index];
                    currentFunctionParameterIndices_[parameter.name] = index;
                    const auto origin = typeMayContainPointer(parameter.type)
                                            ? std::optional<BorrowOrigin>(
                                                  BorrowOrigin{0, parameter.name})
                                            : std::nullopt;
                    env[parameter.name] = {parameter.type, true, 1, origin, nullptr, std::nullopt, false};
                }

                auto savedDiagnostics = std::move(diagnostics_);
                diagnostics_.clear();
                scopeDepth_ = 0;
                currentFunctionReturnType_ = f.ret;
                block(*f.body, env, f.ret, false);
                scopeDepth_ = 0;
                diagnostics_ = std::move(savedDiagnostics);

                auto& known = returnedBorrowParams_[f.name];
                for (const auto index : currentFunctionReturnBorrowParams_) {
                    bool alreadyKnown = false;
                    for (const auto existing : known) {
                        if (existing == index) {
                            alreadyKnown = true;
                            break;
                        }
                    }
                    if (!alreadyKnown) {
                        known.push_back(index);
                        changed = true;
                    }
                }
            }
            if (!changed)
                break;
        }

        // Infer which i64 parameters may be returned as managed allocation
        // handles, whether a function creates a fresh handle, and which parameters
        // may be released. Fixed-point iteration handles forward-declared wrappers.
        for (std::size_t pass = 0; pass <= p.functions.size(); ++pass) {
            bool changed = false;
            for (auto& f : p.functions) {
                if (f.external || !f.body)
                    continue;

                std::unordered_map<std::string, Symbol> env;
                currentFunctionParameterIndices_.clear();
                currentFunctionReturnManagedParams_.clear();
                currentFunctionFreedManagedParams_.clear();
                currentFunctionReturnManagedFresh_ = false;
                currentFunctionReturnManagedUnknown_ = false;
                for (std::size_t index = 0; index < f.params.size(); ++index) {
                    const auto& parameter = f.params[index];
                    currentFunctionParameterIndices_[parameter.name] = index;
                    const auto managedParameter =
                        parameter.type.kind == TypeKind::I64
                            ? std::optional<std::size_t>(index)
                            : std::nullopt;
                    env[parameter.name] = {parameter.type, true, 1, std::nullopt, nullptr,
                                           managedParameter, false};
                }

                auto savedDiagnostics = std::move(diagnostics_);
                diagnostics_.clear();
                scopeDepth_ = 0;
                currentFunctionReturnType_ = f.ret;
                block(*f.body, env, f.ret, false);
                scopeDepth_ = 0;
                diagnostics_ = std::move(savedDiagnostics);

                auto appendUnique = [&](std::vector<std::size_t>& target,
                                        const std::vector<std::size_t>& values) {
                    for (const auto value : values) {
                        if (std::find(target.begin(), target.end(), value) == target.end()) {
                            target.push_back(value);
                            changed = true;
                        }
                    }
                };
                appendUnique(returnedManagedParams_[f.name],
                             currentFunctionReturnManagedParams_);
                appendUnique(freedManagedParams_[f.name], currentFunctionFreedManagedParams_);
                if (currentFunctionReturnManagedFresh_ && !returnedManagedFresh_[f.name]) {
                    returnedManagedFresh_[f.name] = true;
                    changed = true;
                }
            }
            if (!changed)
                break;
        }

        // A return summary is trusted only when every i64 return expression has
        // a recognized managed origin. Propagate incompleteness through wrappers
        // so a mixed/opaque result never masquerades as an allocation handle.
        for (std::size_t pass = 0; pass <= p.functions.size(); ++pass) {
            bool changed = false;
            for (auto& f : p.functions) {
                if (f.external || !f.body || f.ret.kind != TypeKind::I64 ||
                    (returnedManagedUnknown_.contains(f.name) && returnedManagedUnknown_[f.name]))
                    continue;

                std::unordered_map<std::string, Symbol> env;
                currentFunctionParameterIndices_.clear();
                currentFunctionReturnManagedParams_.clear();
                currentFunctionFreedManagedParams_.clear();
                currentFunctionReturnManagedFresh_ = false;
                currentFunctionReturnManagedUnknown_ = false;
                for (std::size_t index = 0; index < f.params.size(); ++index) {
                    const auto& parameter = f.params[index];
                    currentFunctionParameterIndices_[parameter.name] = index;
                    const auto managedParameter =
                        parameter.type.kind == TypeKind::I64
                            ? std::optional<std::size_t>(index)
                            : std::nullopt;
                    env[parameter.name] = {parameter.type, true, 1, std::nullopt, nullptr,
                                           managedParameter, false};
                }

                auto savedDiagnostics = std::move(diagnostics_);
                diagnostics_.clear();
                scopeDepth_ = 0;
                currentFunctionReturnType_ = f.ret;
                block(*f.body, env, f.ret, false);
                scopeDepth_ = 0;
                diagnostics_ = std::move(savedDiagnostics);

                if (currentFunctionReturnManagedUnknown_) {
                    returnedManagedUnknown_[f.name] = true;
                    changed = true;
                }
            }
            if (!changed)
                break;
        }

        // Validate function bodies with the completed summaries. Parameter values
        // carry a depth-zero origin, while taking the address of the parameter
        // itself still points at local stack storage and remains non-escapable.
        for (auto& f : p.functions) {
            if (f.external)
                continue;
            std::unordered_map<std::string, Symbol> env;
            currentFunctionParameterIndices_.clear();
            currentFunctionReturnBorrowParams_.clear();
            currentFunctionReturnManagedParams_.clear();
            currentFunctionFreedManagedParams_.clear();
            currentFunctionReturnManagedFresh_ = false;
            currentFunctionReturnManagedUnknown_ = false;
            for (std::size_t index = 0; index < f.params.size(); ++index) {
                auto& a = f.params[index];
                if (env.contains(a.name)) {
                    report("duplicate parameter: " + a.name, a.span.valid ? a.span : f.span);
                    continue;
                }
                currentFunctionParameterIndices_[a.name] = index;
                const auto origin = typeMayContainPointer(a.type)
                                        ? std::optional<BorrowOrigin>(
                                              BorrowOrigin{0, a.name})
                                        : std::nullopt;
                env[a.name] = {a.type, true, 1, origin, nullptr,
                               a.type.kind == TypeKind::I64
                                   ? std::optional<std::size_t>(index)
                                   : std::nullopt,
                               false};
            }
            if (f.body) {
                currentFunctionReturnType_ = f.ret;
                scopeDepth_ = 0;
                block(*f.body, env, f.ret, false);
                scopeDepth_ = 0;
                if (f.ret.kind != TypeKind::Void && !blockAlwaysReturns(*f.body)) {
                    std::string message = "function may exit without returning a value: " + f.name;
                    if (f.span.valid)
                        message += sourceLocation(f.span);
                    diagnostics_.push_back(std::move(message));
                }
            }
        }

        if (!diagnostics_.empty()) {
            std::ostringstream summary;
            summary << "NEXUS found " << diagnostics_.size() << " semantic error"
                    << (diagnostics_.size() == 1 ? "" : "s") << ":\n";
            for (const auto& diagnostic : diagnostics_)
                summary << "  - " << diagnostic << "\n";
            throw std::runtime_error(summary.str());
        }
    }

} // namespace nexus
