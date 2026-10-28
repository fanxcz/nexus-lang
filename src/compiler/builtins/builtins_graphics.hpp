#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinGraphicsSignature(const std::string& name) {

        using T = Type;

        if (name == "gfx_init")
            return BuiltinSignature{
                name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_gfx_init"};

        if (name == "gfx_error")
            return BuiltinSignature{name, {}, T::string(), "nexus_gfx_error"};

        if (name == "gfx_shutdown")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_shutdown"};

        if (name == "gfx_should_close")
            return BuiltinSignature{name, {}, T::boolean(), "nexus_gfx_should_close"};

        if (name == "gfx_poll")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_poll"};

        if (name == "gfx_event_type")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_type"};

        if (name == "gfx_event_key")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_key"};

        if (name == "gfx_event_mouse_button")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_mouse_button"};

        if (name == "gfx_event_x")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_x"};

        if (name == "gfx_event_y")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_y"};

        if (name == "gfx_event_is")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_event_is"};

        if (name == "gfx_set_ui_resolution")
            return BuiltinSignature{
                name, {T::f64(), T::f64()}, T::void_(), "nexus_gfx_set_ui_resolution"};

        if (name == "gfx_width")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_width"};

        if (name == "gfx_height")
            return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_height"};

        if (name == "gfx_vsync")
            return BuiltinSignature{name, {T::boolean()}, T::boolean(), "nexus_gfx_vsync"};

        if (name == "gfx_mouse_x")
            return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_mouse_x"};

        if (name == "gfx_mouse_y")
            return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_mouse_y"};

        if (name == "gfx_mouse_down")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_mouse_down"};

        if (name == "gfx_mouse_clicked")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_mouse_clicked"};

        if (name == "gfx_key_down")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_key_down"};

        if (name == "gfx_key_pressed")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_key_pressed"};

        if (name == "gfx_dt")
            return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_dt"};

        if (name == "gfx_begin")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_begin"};

        if (name == "gfx_end")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_end"};

        if (name == "gfx_clear")
            return BuiltinSignature{
                name, {T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_clear"};

        if (name == "gfx_rect")
            return BuiltinSignature{
                name,
                {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_rect"};

        if (name == "gfx_circle")
            return BuiltinSignature{
                name,
                {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_circle"};

        if (name == "gfx_line")
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
                                    "nexus_gfx_line"};

        if (name == "gfx_text")
            return BuiltinSignature{
                name,
                {T::f64(), T::f64(), T::f64(), T::string(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_text"};

        if (name == "gfx_set_title")
            return BuiltinSignature{name, {T::string()}, T::void_(), "nexus_gfx_set_title"};

        if (name == "gfx_fullscreen")
            return BuiltinSignature{name, {T::boolean()}, T::void_(), "nexus_gfx_fullscreen"};

        if (name == "gfx_maximize")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_maximize"};

        if (name == "gfx_restore")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_restore"};

        if (name == "gfx_is_fullscreen")
            return BuiltinSignature{name, {}, T::boolean(), "nexus_gfx_is_fullscreen"};

        if (name == "gfx_backend")
            return BuiltinSignature{name, {}, T::string(), "nexus_gfx_backend"};

        if (name == "gfx_texture_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_texture_load"};

        if (name == "gfx_texture_draw")
            return BuiltinSignature{name,
                                    {T::i64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_gfx_texture_draw"};

        if (name == "gfx_texture_unload")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_texture_unload"};

        if (name == "gfx_texture_width")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_texture_width"};

        if (name == "gfx_texture_height")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_texture_height"};

        if (name == "gfx_canvas_create")
            return BuiltinSignature{name,
                                    {T::i64(), T::i64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::i64(),
                                    "nexus_gfx_canvas_create"};

        if (name == "gfx_canvas_destroy")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_canvas_destroy"};

        if (name == "gfx_canvas_clear")
            return BuiltinSignature{name,
                                    {T::i64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::void_(),
                                    "nexus_gfx_canvas_clear"};

        if (name == "gfx_canvas_brush")
            return BuiltinSignature{
                name,
                {T::i64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_canvas_brush"};

        if (name == "gfx_canvas_line")
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
                                    "nexus_gfx_canvas_line"};

        if (name == "gfx_canvas_rect")
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
                                     T::f64(),
                                     T::boolean()},
                                    T::void_(),
                                    "nexus_gfx_canvas_rect"};

        if (name == "gfx_canvas_fill")
            return BuiltinSignature{
                name,
                {T::i64(), T::i64(), T::i64(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_canvas_fill"};

        if (name == "gfx_canvas_draw")
            return BuiltinSignature{name,
                                    {T::i64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()},
                                    T::void_(),
                                    "nexus_gfx_canvas_draw"};

        if (name == "gfx_canvas_save_bmp")
            return BuiltinSignature{
                name, {T::i64(), T::string()}, T::boolean(), "nexus_gfx_canvas_save_bmp"};

        if (name == "gfx_canvas_load_bmp")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_canvas_load_bmp"};

        if (name == "gfx_canvas_checkpoint")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_canvas_checkpoint"};

        if (name == "gfx_canvas_undo")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_canvas_undo"};

        if (name == "gfx_canvas_redo")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_canvas_redo"};

        if (name == "gfx_canvas_width")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_canvas_width"};

        if (name == "gfx_canvas_height")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_canvas_height"};

        if (name == "gfx_particle_create")
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
                                    T::i64(),
                                    "nexus_gfx_particle_create"};

        if (name == "gfx_particle_alive")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_particle_alive"};

        if (name == "gfx_particles_update")
            return BuiltinSignature{name, {T::f64()}, T::void_(), "nexus_gfx_particles_update"};

        if (name == "gfx_particles_draw")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_particles_draw"};

        if (name == "gfx_particle_destroy")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_particle_destroy"};

        if (name == "gfx_particles_clear")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_particles_clear"};

        if (name == "gfx_camera_set")
            return BuiltinSignature{
                name, {T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_camera_set"};

        if (name == "gfx_camera_reset")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_camera_reset"};

        if (name == "gfx_texture_draw_frame")
            return BuiltinSignature{name,
                                    {T::i64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::i64(),
                                     T::i64(),
                                     T::i64(),
                                     T::i64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_gfx_texture_draw_frame"};

        if (name == "gfx_image_type")
            return BuiltinSignature{name, {}, T::string(), "nexus_gfx_image_type"};

        if (name == "gfx_font_load")
            return BuiltinSignature{name, {T::string(), T::i64()}, T::i64(), "nexus_gfx_font_load"};

        if (name == "gfx_text_font")
            return BuiltinSignature{
                name,
                {T::i64(), T::f64(), T::f64(), T::string(), T::f64(), T::f64(), T::f64(), T::f64()},
                T::void_(),
                "nexus_gfx_text_font"};

        if (name == "gfx_font_unload")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_font_unload"};

        if (name == "gfx_font_default")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_font_default"};

        if (name == "gfx_text_width")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::i64(), "nexus_gfx_text_width"};

        if (name == "gfx_text_height")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_text_height"};

        if (name == "gfx_font_style")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::void_(), "nexus_gfx_font_style"};

        if (name == "gfx_cursor_visible")
            return BuiltinSignature{name, {T::boolean()}, T::void_(), "nexus_gfx_cursor_visible"};

        if (name == "gfx_texture_filter")
            return BuiltinSignature{
                name, {T::i64(), T::boolean()}, T::void_(), "nexus_gfx_texture_filter"};

        if (name == "gfx_anim_create")
            return BuiltinSignature{
                name,
                {T::i64(), T::i64(), T::i64(), T::i64(), T::i64(), T::f64(), T::boolean()},
                T::i64(),
                "nexus_gfx_anim_create"};

        if (name == "gfx_anim_update")
            return BuiltinSignature{
                name, {T::i64(), T::f64()}, T::void_(), "nexus_gfx_anim_update"};

        if (name == "gfx_anim_draw")
            return BuiltinSignature{name,
                                    {T::i64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64(),
                                     T::f64()},
                                    T::void_(),
                                    "nexus_gfx_anim_draw"};

        if (name == "gfx_anim_frame")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_anim_frame"};

        if (name == "gfx_anim_destroy")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_anim_destroy"};

        return std::nullopt;
    }

} // namespace nexus
