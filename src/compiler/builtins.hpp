#pragma once
#include "builtins/builtin_signature.hpp"
#include "builtins/builtins_core.hpp"
#include "builtins/builtins_strings.hpp"
#include "builtins/builtins_collections.hpp"
#include "builtins/builtins_graphics.hpp"
#include "builtins/builtins_screen.hpp"
#include "builtins/builtins_ui.hpp"
#include "builtins/builtins_audio.hpp"
#include "builtins/builtins_game.hpp"
#include "builtins/builtins_math.hpp"
#include "builtins/builtins_data.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinSignature(const std::string& name) {
        if (auto signature = builtinCoreSignature(name))
            return signature;
        if (auto signature = builtinStringSignature(name))
            return signature;
        if (auto signature = builtinCollectionSignature(name))
            return signature;
        if (auto signature = builtinGraphicsSignature(name))
            return signature;
        if (auto signature = builtinScreenSignature(name))
            return signature;
        if (auto signature = builtinUiSignature(name))
            return signature;
        if (auto signature = builtinAudioSignature(name))
            return signature;
        if (auto signature = builtinGameSignature(name))
            return signature;
        if (auto signature = builtinMathSignature(name))
            return signature;
        if (auto signature = builtinDataSignature(name))
            return signature;
        return std::nullopt;
    }

} // namespace nexus
