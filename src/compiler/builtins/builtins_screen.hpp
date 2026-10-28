#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinScreenSignature(const std::string& name) {

        using T = Type;

        if (name == "screen_begin")
            return BuiltinSignature{name, {}, T::void_(), "nexus_screen_begin"};

        if (name == "screen_end")
            return BuiltinSignature{name, {}, T::void_(), "nexus_screen_end"};

        if (name == "screen_clear")
            return BuiltinSignature{name, {}, T::void_(), "nexus_screen_clear"};

        if (name == "screen_put")
            return BuiltinSignature{
                name, {T::i64(), T::i64(), T::string()}, T::void_(), "nexus_screen_put"};

        if (name == "screen_present")
            return BuiltinSignature{name, {}, T::void_(), "nexus_screen_present"};

        if (name == "screen_set_title")
            return BuiltinSignature{name, {T::string()}, T::void_(), "nexus_screen_set_title"};

        if (name == "screen_width")
            return BuiltinSignature{name, {}, T::i64(), "nexus_screen_width"};

        if (name == "screen_height")
            return BuiltinSignature{name, {}, T::i64(), "nexus_screen_height"};

        if (name == "beep")
            return BuiltinSignature{name, {}, T::void_(), "nexus_beep"};

        return std::nullopt;
    }

} // namespace nexus
