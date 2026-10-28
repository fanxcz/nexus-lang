#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinMathSignature(const std::string& name) {

        using T = Type;

        if (name == "sqrt")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_sqrt"};

        // Core floating-point math used by 2D/3D games, animation and simulation.

        if (name == "sin")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_sin"};

        if (name == "cos")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_cos"};

        if (name == "tan")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_tan"};

        if (name == "atan2")
            return BuiltinSignature{name, {T::f64(), T::f64()}, T::f64(), "nexus_atan2"};

        if (name == "floor")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_floor"};

        if (name == "ceil")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_ceil"};

        if (name == "pow")
            return BuiltinSignature{name, {T::f64(), T::f64()}, T::f64(), "nexus_pow"};

        return std::nullopt;
    }

} // namespace nexus
