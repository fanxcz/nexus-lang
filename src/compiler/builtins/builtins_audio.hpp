#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinAudioSignature(const std::string& name) {

        using T = Type;

        if (name == "gfx_sound_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_sound_load"};

        if (name == "gfx_sound_play")
            return BuiltinSignature{
                name, {T::i64(), T::boolean()}, T::void_(), "nexus_gfx_sound_play"};

        if (name == "gfx_sound_stop")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_sound_stop"};

        if (name == "gfx_sound_unload")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_sound_unload"};

        if (name == "gfx_audio_available")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_audio_available"};

        if (name == "gfx_audio_error")
            return BuiltinSignature{name, {}, T::string(), "nexus_gfx_audio_error"};

        if (name == "gfx_sound_volume")
            return BuiltinSignature{name, {T::f64()}, T::void_(), "nexus_gfx_sound_volume"};

        if (name == "gfx_sound_playing")
            return BuiltinSignature{name, {}, T::boolean(), "nexus_gfx_sound_playing"};

        if (name == "gfx_sound_tone")
            return BuiltinSignature{
                name, {T::f64(), T::i64(), T::f64()}, T::void_(), "nexus_gfx_sound_tone"};

        if (name == "gfx_music_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_music_load"};

        if (name == "gfx_music_play")
            return BuiltinSignature{
                name, {T::i64(), T::boolean()}, T::boolean(), "nexus_gfx_music_play"};

        if (name == "gfx_music_stop")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_music_stop"};

        if (name == "gfx_music_volume")
            return BuiltinSignature{name, {T::f64()}, T::void_(), "nexus_gfx_music_volume"};

        return std::nullopt;
    }

} // namespace nexus
