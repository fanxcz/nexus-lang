#pragma once

#include "ast/ast.hpp"

#include <string>
#include <vector>

namespace nexus {

    struct BuiltinSignature {

        std::string name;

        std::vector<Type> params;

        Type ret;

        std::string runtimeName;
    };

} // namespace nexus
