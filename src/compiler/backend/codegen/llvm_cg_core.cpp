#include "codegen/llvm_cg.hpp"

#include <stdexcept>

namespace nexus {

    CG::CG(CodegenOptions o) : options(std::move(o)) {}

    std::string CG::next() {
        return "%" + std::to_string(++reg);
    }
    std::string CG::lab(const std::string& p) {
        return p + std::to_string(++label);
    }
    void CG::emit(const std::string& s) {
        body << s << '\n';
    }
    void CG::startBlock(const std::string& name) {
        emit(name + ":");
        currentBlock = name;
    }
    void CG::emitModule(const std::string& s) {
        module << s << '\n';
    }
    std::string CG::llvmType(const Type& t) {
        switch (t.kind) {
        case TypeKind::I8: return "i8";
        case TypeKind::I16: return "i16";
        case TypeKind::I32: return "i32";
        case TypeKind::I64: return "i64";
        case TypeKind::U8: return "i8";
        case TypeKind::U16: return "i16";
        case TypeKind::U32: return "i32";
        case TypeKind::U64: return "i64";
        case TypeKind::F32: return "float";
        case TypeKind::F64: return "double";
        case TypeKind::Bool:
            return "i1";
        case TypeKind::String:
            return "ptr";
        case TypeKind::Void:
            return "void";
        case TypeKind::Pointer:
            return "ptr";
        case TypeKind::Struct:
            return "%struct." + t.name;
        case TypeKind::Enum:
            return "{ i64, i64 }";
        case TypeKind::Array:
            return "[" + std::to_string(t.arraySize) + " x " + llvmType(*t.element) + "]";
        case TypeKind::Slice:
        case TypeKind::MutableSlice:
            // Slices are non-owning views: element pointer plus element count.
            return "{ ptr, i64 }";
        default:
            return "i64";
        }
    }
    std::string CG::escape(const std::string& s) {
        std::ostringstream q;
        for (unsigned char c : s) {
            switch (c) {
            case '\\':
                q << "\\5C";
                break;
            case '"':
                q << "\\22";
                break;
            case '\n':
                q << "\\0A";
                break;
            case '\r':
                q << "\\0D";
                break;
            case '\t':
                q << "\\09";
                break;
            default:
                if (c < 32) {
                    char b[5];
                    std::snprintf(b, sizeof b, "\\%02X", c);
                    q << b;
                } else
                    q << c;
            }
        }
        return q.str();
    }
    const StructDecl& CG::getStruct(const std::string& n) const {
        auto it = structs.find(n);
        if (it == structs.end())
            throw std::runtime_error("unknown struct in codegen: " + n);
        return *it->second;
    }
    const EnumDecl& CG::getEnum(const std::string& n) const {
        auto it = enums.find(n);
        if (it == enums.end())
            throw std::runtime_error("unknown enum in codegen: " + n);
        return *it->second;
    }
    std::size_t CG::fieldIndex(const StructDecl& s, const std::string& n) const {
        for (std::size_t i = 0; i < s.fields.size(); ++i)
            if (s.fields[i].name == n)
                return i;
        throw std::runtime_error("unknown field in codegen: " + n);
    }
    long long CG::enumTag(const EnumVariantExpr& e) const {
        const auto& en = getEnum(e.enumName);
        for (std::size_t i = 0; i < en.variants.size(); ++i)
            if (en.variants[i].name == e.variant)
                return static_cast<long long>(i);
        throw std::runtime_error("unknown enum variant in codegen");
    }

} // namespace nexus
