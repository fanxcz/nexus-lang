#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinCoreSignature(const std::string& name) {

        using T = Type;

        if (name == "input")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_input"};

        if (name == "input_i64")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_input_i64"};

        if (name == "input_f64")
            return BuiltinSignature{name, {T::string()}, T::f64(), "nexus_input_f64"};

        if (name == "str_i64")
            return BuiltinSignature{name, {T::i64()}, T::string(), "nexus_str_i64"};

        if (name == "str_f64")
            return BuiltinSignature{name, {T::f64()}, T::string(), "nexus_str_f64"};

        if (name == "str_bool")
            return BuiltinSignature{name, {T::boolean()}, T::string(), "nexus_str_bool"};

        if (name == "to_f64")
            return BuiltinSignature{name, {T::i64()}, T::f64(), "nexus_to_f64"};

        if (name == "to_i64")
            return BuiltinSignature{name, {T::f64()}, T::i64(), "nexus_to_i64"};

        if (name == "read_key")
            return BuiltinSignature{name, {}, T::i64(), "nexus_read_key"};

        if (name == "key_pressed")
            return BuiltinSignature{name, {}, T::boolean(), "nexus_key_pressed"};

        if (name == "clear")
            return BuiltinSignature{name, {}, T::void_(), "nexus_clear"};

        if (name == "sleep")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_sleep_ms"};

        if (name == "random_i64")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::i64(), "nexus_random_i64"};

        if (name == "time_ms")
            return BuiltinSignature{name, {}, T::i64(), "nexus_time_ms"};

        if (name == "system")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_system"};

        if (name == "capture_output")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_capture_output"};

        if (name == "file_read")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_file_read"};

        if (name == "file_read_checked")
            return BuiltinSignature{name,
                                    {T::string(), T::ptr(T::string())},
                                    T::boolean(),
                                    "nexus_file_read_checked"};

        if (name == "file_write")
            return BuiltinSignature{name, {T::string(), T::string()}, T::i64(), "nexus_file_write"};

        if (name == "file_exists")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_file_exists"};

        if (name == "env")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_env"};

        if (name == "exit")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_exit"};

        if (name == "os.args_count")
            return BuiltinSignature{name, {}, T::i64(), "nexus_os_args_count"};

        if (name == "os.args_get")
            return BuiltinSignature{name, {T::i64()}, T::string(), "nexus_os_args_get"};

        if (name == "gfx3d_begin")
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
                                    "nexus_gfx3d_begin"};

        if (name == "gfx3d_cube")
            return BuiltinSignature{name,
                                    {T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_gfx3d_cube"};

        if (name == "gfx3d_grid")
            return BuiltinSignature{name,
                                    {T::i64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::void_(),
                                    "nexus_gfx3d_grid"};

        if (name == "gfx3d_end")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx3d_end"};

        if (name == "gfx3d_model_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx3d_model_load"};

        if (name == "gfx3d_model_draw")
            return BuiltinSignature{name,
                                    {T::i64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_gfx3d_model_draw"};

        if (name == "gfx3d_model_unload")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx3d_model_unload"};

        if (name == "mem_alloc")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_mem_alloc"};

        if (name == "mem_free")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_mem_free"};

        if (name == "mem_read")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_mem_read"};

        if (name == "mem_set")
            return BuiltinSignature{
                name, {T::i64(), T::i64(), T::i64()}, T::void_(), "nexus_mem_set"};

        if (name == "mem_copy")
            return BuiltinSignature{
                name, {T::i64(), T::i64(), T::i64()}, T::void_(), "nexus_mem_copy"};

        return std::nullopt;
    }

} // namespace nexus
