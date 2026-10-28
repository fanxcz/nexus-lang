#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinGameSignature(const std::string& name) {

        using T = Type;

        if (name == "physics_aabb")
            return BuiltinSignature{
                name,
                {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::boolean(),
                "nexus_physics_aabb"};

        if (name == "physics_circle")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::boolean(),
                                    "nexus_physics_circle"};

        if (name == "scene_create")
            return BuiltinSignature{name, {}, T::i64(), "nexus_scene_create"};

        if (name == "scene_add")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::void_(), "nexus_scene_add"};

        if (name == "scene_save")
            return BuiltinSignature{
                name, {T::i64(), T::string()}, T::boolean(), "nexus_scene_save"};

        if (name == "scene_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_scene_load"};

        if (name == "scene_destroy")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_scene_destroy"};

        if (name == "scene_clear")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_scene_clear"};

        if (name == "ecs_create")
            return BuiltinSignature{name, {}, T::i64(), "nexus_ecs_create"};

        if (name == "ecs_destroy")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_ecs_destroy"};

        if (name == "ecs_alive")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_ecs_alive"};

        if (name == "ecs_set_position")
            return BuiltinSignature{
                name, {T::i64(), T::f64(), T::f64()}, T::void_(), "nexus_ecs_set_position"};

        if (name == "ecs_set_velocity")
            return BuiltinSignature{
                name, {T::i64(), T::f64(), T::f64()}, T::void_(), "nexus_ecs_set_velocity"};

        if (name == "ecs_set_size")
            return BuiltinSignature{
                name, {T::i64(), T::f64(), T::f64()}, T::void_(), "nexus_ecs_set_size"};

        if (name == "ecs_set_color")
            return BuiltinSignature{name,
                                    {T::i64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::void_(),
                                    "nexus_ecs_set_color"};

        if (name == "ecs_set_texture")
            return BuiltinSignature{
                name, {T::i64(), T::i64()}, T::void_(), "nexus_ecs_set_texture"};

        if (name == "ecs_update")
            return BuiltinSignature{name, {T::f64()}, T::void_(), "nexus_ecs_update"};

        if (name == "ecs_draw")
            return BuiltinSignature{name, {}, T::void_(), "nexus_ecs_draw"};

        if (name == "ecs_collides")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_ecs_collides"};

        if (name == "ecs_x")
            return BuiltinSignature{name, {T::i64()}, T::f64(), "nexus_ecs_x"};

        if (name == "ecs_y")
            return BuiltinSignature{name, {T::i64()}, T::f64(), "nexus_ecs_y"};

        if (name == "ecs_count")
            return BuiltinSignature{name, {}, T::i64(), "nexus_ecs_count"};

        return std::nullopt;
    }

} // namespace nexus
