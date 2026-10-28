#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinStringSignature(const std::string& name) {

        using T = Type;

        if (name == "string.len")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_string_len"};

        if (name == "string.byte_at")
            return BuiltinSignature{
                name, {T::string(), T::i64()}, T::i64(), "nexus_string_byte_at"};

        if (name == "string.chars")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_string_char_count"};

        if (name == "string.char_at")
            return BuiltinSignature{
                name, {T::string(), T::i64()}, T::i64(), "nexus_string_char_at"};

        if (name == "string.from_char")
            return BuiltinSignature{name, {T::i64()}, T::string(), "nexus_string_from_char"};

        if (name == "string.valid_utf8")
            return BuiltinSignature{
                name, {T::string()}, T::boolean(), "nexus_string_valid_utf8"};

        if (name == "string.contains")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::boolean(), "nexus_string_contains"};

        if (name == "string.find")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::i64(), "nexus_string_find"};

        if (name == "string.trim")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_string_trim"};

        if (name == "string.upper")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_string_upper"};

        if (name == "string.lower")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_string_lower"};

        if (name == "string.slice")
            return BuiltinSignature{
                name, {T::string(), T::i64(), T::i64()}, T::string(), "nexus_string_slice"};

        if (name == "string.split_count")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::i64(), "nexus_string_split_count"};

        if (name == "string.split_at")
            return BuiltinSignature{
                name,
                {T::string(), T::string(), T::i64()},
                T::string(),
                "nexus_string_split_at"};

        return std::nullopt;
    }

} // namespace nexus
