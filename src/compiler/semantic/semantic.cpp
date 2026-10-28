#include "semantic/semantic.hpp"

#include "builtins.hpp"
#include "semantic/semantic_internal.hpp"

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
        funcs_.clear();
        structs_.clear();
        enums_.clear();

        const auto located = [](const std::string& message, const SourceSpan& span) {
            if (!span.valid || message.rfind(" at ") != std::string::npos)
                return message;
            return message + " at " + std::to_string(span.start.line) + ":" +
                   std::to_string(span.start.column);
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
        std::function<void(const Type&)> validateType;

        validateType = [&](const Type& t) {
            if (t.kind == TypeKind::Struct && !structs_.contains(t.name))
                throw std::runtime_error("unknown type: " + t.name);

            if (t.kind == TypeKind::Enum && !enums_.contains(t.name))
                throw std::runtime_error("unknown enum: " + t.name);

            if (t.kind == TypeKind::Pointer && t.pointee) {
                validateType(*t.pointee);
            }
            if (t.kind == TypeKind::Array && t.element) {
                validateType(*t.element);
            }
        };

        const auto validateTypeAt = [&](const Type& type, const SourceSpan& span) {
            try {
                validateType(type);
            } catch (const std::exception& error) {
                report(error.what(), span);
            }
        };

        for (auto& e : p.enums) {
            for (auto& variant : e.variants) {
                if (!variant.payload)
                    continue;
                normalize(*variant.payload);
                validateTypeAt(*variant.payload, variant.span.valid ? variant.span : e.span);
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
                validateTypeAt(field.type, field.span.valid ? field.span : s.span);
            }

        for (auto& f : p.functions) {
            for (auto& a : f.params) {
                normalize(a.type);
                validateTypeAt(a.type, a.span.valid ? a.span : f.span);
            }
            normalize(f.ret);
            validateTypeAt(f.ret, f.span);
            if (f.external)
                continue;
            std::unordered_map<std::string, Symbol> env;
            for (auto& a : f.params) {
                if (env.contains(a.name)) {
                    report("duplicate parameter: " + a.name, a.span.valid ? a.span : f.span);
                    continue;
                }
                env[a.name] = {a.type, true};
            }
            if (f.body) {
                currentFunctionReturnType_ = f.ret;
                block(*f.body, env, f.ret, false);
                if (f.ret.kind != TypeKind::Void && !blockAlwaysReturns(*f.body)) {
                    std::string message = "function may exit without returning a value: " + f.name;
                    if (f.span.valid)
                        message += " at " + std::to_string(f.span.start.line) + ":" +
                                   std::to_string(f.span.start.column);
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
