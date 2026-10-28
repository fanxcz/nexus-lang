#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinUiSignature(const std::string& name) {

        using T = Type;

        if (name == "ui_button")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::f64(), T::f64(), T::string()},
                                    T::boolean(),
                                    "nexus_ui_button"};

        if (name == "ui_text_field")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::f64(), T::f64(),
                                     T::string(), T::string(), T::i64()},
                                    T::string(),
                                    "nexus_ui_text_field"};

        if (name == "ui_panel")
            return BuiltinSignature{
                name,
                {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_ui_panel"};

        if (name == "ui_label")
            return BuiltinSignature{
                name, {T::f64(), T::f64(), T::string(), T::f64()}, T::void_(), "nexus_ui_label"};

        if (name == "ui_progress")
            return BuiltinSignature{name,
                                    {T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_ui_progress"};

        if (name == "ui_checkbox")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::string(), T::boolean()},
                                    T::boolean(),
                                    "nexus_ui_checkbox"};

        if (name == "ui_slider")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::f64(),
                                    "nexus_ui_slider"};

        return std::nullopt;
    }

} // namespace nexus
