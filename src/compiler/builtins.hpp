#pragma once
#include "ast/ast.hpp"

#include <optional>
#include <string>
#include <vector>

namespace nexus {

    struct BuiltinSignature {

        std::string name;

        std::vector<Type> params;

        Type ret;

        std::string runtimeName;
    };

    inline std::optional<BuiltinSignature> builtinSignature(const std::string& name) {

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

        if (name == "ilist_new")
            return BuiltinSignature{name, {}, T::i64(), "nexus_ilist_new"};

        if (name == "ilist_push")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::void_(), "nexus_ilist_push"};

        if (name == "ilist_len")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_ilist_len"};

        if (name == "ilist_get")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::i64(), "nexus_ilist_get"};

        if (name == "ilist_set")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::void_(), "nexus_ilist_set"};

        if (name == "ilist_push_checked")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_ilist_push_checked"};
        if (name == "ilist_reserve")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_ilist_reserve"};
        if (name == "ilist_clear")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_ilist_clear"};
        if (name == "ilist_free")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_ilist_free"};
        if (name == "ilist_insert")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::boolean(), "nexus_ilist_insert"};
        if (name == "ilist_try_get")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_try_get"};
        if (name == "ilist_set_checked")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::boolean(), "nexus_ilist_set_checked"};
        if (name == "ilist_remove")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_remove"};
        if (name == "ilist_try_pop")
            return BuiltinSignature{name, {T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_try_pop"};

        if (name == "slist_new")
            return BuiltinSignature{name, {}, T::i64(), "nexus_slist_new"};

        if (name == "slist_push")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::void_(), "nexus_slist_push"};

        if (name == "slist_len")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_slist_len"};

        if (name == "slist_get")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::string(), "nexus_slist_get"};

        if (name == "slist_push_checked")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::boolean(), "nexus_slist_push_checked"};
        if (name == "slist_reserve")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_slist_reserve"};
        if (name == "slist_clear")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_slist_clear"};
        if (name == "slist_free")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_slist_free"};
        if (name == "slist_insert")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_slist_insert"};
        if (name == "slist_try_get")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_try_get"};
        if (name == "slist_set_checked")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_slist_set_checked"};
        if (name == "slist_remove")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_remove"};
        if (name == "slist_try_pop")
            return BuiltinSignature{name, {T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_try_pop"};

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

        if (name == "gfx_key_down")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_key_down"};

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

        if (name == "sqrt")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "nexus_sqrt"};

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

        if (name == "gfx_music_load")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_music_load"};

        if (name == "gfx_music_play")
            return BuiltinSignature{
                name, {T::i64(), T::boolean()}, T::boolean(), "nexus_gfx_music_play"};

        if (name == "gfx_music_stop")
            return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_music_stop"};

        if (name == "gfx_music_volume")
            return BuiltinSignature{name, {T::f64()}, T::void_(), "nexus_gfx_music_volume"};

        if (name == "gfx_cursor_visible")
            return BuiltinSignature{name, {T::boolean()}, T::void_(), "nexus_gfx_cursor_visible"};

        if (name == "ui_button")
            return BuiltinSignature{name,
                                    {T::f64(), T::f64(), T::f64(), T::f64(), T::string()},
                                    T::boolean(),
                                    "nexus_ui_button"};

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

        if (name == "http_get")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_http_get"};

        if (name == "json_get")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::string(), "nexus_json_get"};

        if (name == "json_valid")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_json_valid"};

        if (name == "json_type")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::string(), "nexus_json_type"};

        if (name == "json_array_len")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::i64(), "nexus_json_array_len"};

        if (name == "json_array_get")
            return BuiltinSignature{
                name, {T::string(), T::string(), T::i64()}, T::string(), "nexus_json_array_get"};

        if (name == "sqlite_open")
            return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_sqlite_open"};

        if (name == "sqlite_exec")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::i64(), "nexus_sqlite_exec"};

        if (name == "sqlite_close")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_sqlite_close"};

        if (name == "sha256")
            return BuiltinSignature{name, {T::string()}, T::string(), "nexus_sha256"};

        return std::nullopt;
    }

} // namespace nexus
