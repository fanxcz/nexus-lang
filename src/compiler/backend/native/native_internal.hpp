#pragma once

#include "native/native.hpp"

#include <map>
#include <string>
#include <vector>

namespace nexus::native {
    namespace detail {

        enum class Kind { I64, Bool, Void, Str };

        struct FnSig {
            std::vector<Kind> params;
            Kind ret = Kind::Void;
        };

        bool kindOfType(const Type& t, Kind& out);
        Kind checkExpr(const Expr* e,
                       const std::map<std::string, Kind>& env,
                       const std::map<std::string, FnSig>& fns,
                       std::string& reason);
        void checkBlock(const Block* b,
                        std::map<std::string, Kind>& env,
                        const std::map<std::string, FnSig>& fns,
                        bool inLoop,
                        std::string& reason);

    } // namespace detail
} // namespace nexus::native
