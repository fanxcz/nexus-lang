#include "codegen/llvm_cg.hpp"

#include <stdexcept>

namespace nexus {

    std::string CG::generate(const Program& p) {

        for (auto& s : const_cast<Program&>(p).structs)
            structs[s.name] = &s;
        for (auto& e : const_cast<Program&>(p).enums)
            enums[e.name] = &e;
        for (auto& f : const_cast<Program&>(p).functions)
            functions[f.name] = &f;

        emitModule("; Nexus LLVM IR v1.0.0");
        if (!options.targetTriple.empty())
            emitModule("target triple = \"" + options.targetTriple + "\"");

        for (auto& s : p.structs) {
            std::ostringstream t;
            t << "%struct." << s.name << " = type { ";
            for (size_t i = 0; i < s.fields.size(); ++i) {
                if (i)
                    t << ", ";
                t << llvmType(s.fields[i].type);
            }
            t << " }";
            emitModule(t.str());
        }
        emitModule("declare void @nexus_print_i64(i64)");
        emitModule("declare void @nexus_print_f64(double)");
        emitModule("declare void @nexus_print_bool(i1)");
        emitModule("declare void @nexus_print_str(ptr)");
        emitModule("declare ptr @nexus_str_concat(ptr, ptr)");
        emitModule("declare i1 @nexus_str_equal(ptr, ptr)");
        emitModule("declare void @nexus_bounds_check(i64, i64)");
        emitModule("declare void @nexus_division_check(i64, i64)");
        emitModule("declare ptr @nexus_input(ptr)");
        emitModule("declare i64 @nexus_input_i64(ptr)");
        emitModule("declare double @nexus_input_f64(ptr)");
        emitModule("declare ptr @nexus_str_i64(i64)");
        emitModule("declare ptr @nexus_str_f64(double)");
        emitModule("declare ptr @nexus_str_bool(i1)");
        emitModule("declare i64 @nexus_read_key()");
        emitModule("declare i1 @nexus_key_pressed()");
        emitModule("declare void @nexus_clear()");
        emitModule("declare void @nexus_sleep_ms(i64)");
        emitModule("declare i64 @nexus_random_i64(i64, i64)");
        emitModule("declare i64 @nexus_time_ms()");
        emitModule("declare i64 @nexus_system(ptr)");
        emitModule("declare ptr @nexus_capture_output(ptr)");
        emitModule("declare ptr @nexus_file_read(ptr)");
        emitModule("declare i1 @nexus_file_read_checked(ptr, ptr)");
        emitModule("declare i64 @nexus_file_write(ptr, ptr)");
        emitModule("declare i1 @nexus_file_exists(ptr)");
        emitModule("declare ptr @nexus_env(ptr)");
        emitModule("declare void @nexus_exit(i64)");
        emitModule("declare void @nexus_screen_begin()");
        emitModule("declare void @nexus_screen_end()");
        emitModule("declare void @nexus_screen_clear()");
        emitModule("declare void @nexus_screen_put(i64, i64, ptr)");
        emitModule("declare void @nexus_screen_present()");
        emitModule("declare void @nexus_screen_set_title(ptr)");
        emitModule("declare i64 @nexus_screen_width()");
        emitModule("declare i64 @nexus_screen_height()");
        emitModule("declare void @nexus_beep()");
        emitModule("declare i64 @nexus_mem_alloc(i64)");
        emitModule("declare void @nexus_mem_free(i64)");
        emitModule("declare void @nexus_mem_set(i64,i64,i64)");
        emitModule("declare i64 @nexus_mem_read(i64)");
        emitModule("declare void @nexus_mem_copy(i64,i64,i64)");
        emitModule("declare ptr @nexus_http_get(ptr)");
        emitModule("declare ptr @nexus_json_get(ptr, ptr)");
        emitModule("declare i64 @nexus_sqlite_open(ptr)");
        emitModule("declare i64 @nexus_sqlite_exec(i64, ptr)");
        emitModule("declare void @nexus_sqlite_close(i64)");
        emitModule("declare ptr @nexus_sha256(ptr)");
        emitModule("declare i64 @nexus_os_args_count()");
        emitModule("declare ptr @nexus_os_args_get(i64)");
        emitModule("declare i64 @nexus_string_len(ptr)");
        emitModule("declare i64 @nexus_string_byte_at(ptr, i64)");
        emitModule("declare i64 @nexus_string_char_count(ptr)");
        emitModule("declare i64 @nexus_string_char_at(ptr, i64)");
        emitModule("declare ptr @nexus_string_from_char(i64)");
        emitModule("declare i1 @nexus_string_valid_utf8(ptr)");
        emitModule("declare i1 @nexus_string_contains(ptr, ptr)");
        emitModule("declare i64 @nexus_string_find(ptr, ptr)");
        emitModule("declare ptr @nexus_string_trim(ptr)");
        emitModule("declare ptr @nexus_string_upper(ptr)");
        emitModule("declare ptr @nexus_string_lower(ptr)");
        emitModule("declare ptr @nexus_string_slice(ptr, i64, i64)");
        emitModule("declare i64 @nexus_string_split_count(ptr, ptr)");
        emitModule("declare ptr @nexus_string_split_at(ptr, ptr, i64)");
        emitModule("declare i64 @nexus_ilist_new()");
        emitModule("declare void @nexus_ilist_push(i64, i64)");
        emitModule("declare i64 @nexus_ilist_len(i64)");
        emitModule("declare i64 @nexus_ilist_get(i64, i64)");
        emitModule("declare void @nexus_ilist_set(i64, i64, i64)");
        emitModule("declare i64 @nexus_slist_new()");
        emitModule("declare void @nexus_slist_push(i64, ptr)");
        emitModule("declare i64 @nexus_slist_len(i64)");
        emitModule("declare ptr @nexus_slist_get(i64, i64)");
        emitModule("declare i1 @nexus_ilist_push_checked(i64, i64)");
        emitModule("declare i1 @nexus_ilist_reserve(i64, i64)");
        emitModule("declare i1 @nexus_ilist_clear(i64)");
        emitModule("declare i1 @nexus_ilist_free(i64)");
        emitModule("declare i1 @nexus_ilist_insert(i64, i64, i64)");
        emitModule("declare i1 @nexus_ilist_try_get(i64, i64, ptr)");
        emitModule("declare i1 @nexus_ilist_set_checked(i64, i64, i64)");
        emitModule("declare i1 @nexus_ilist_remove(i64, i64, ptr)");
        emitModule("declare i1 @nexus_ilist_try_pop(i64, ptr)");
        emitModule("declare i1 @nexus_slist_push_checked(i64, ptr)");
        emitModule("declare i1 @nexus_slist_reserve(i64, i64)");
        emitModule("declare i1 @nexus_slist_clear(i64)");
        emitModule("declare i1 @nexus_slist_free(i64)");
        emitModule("declare i1 @nexus_slist_insert(i64, i64, ptr)");
        emitModule("declare i1 @nexus_slist_try_get(i64, i64, ptr)");
        emitModule("declare i1 @nexus_slist_set_checked(i64, i64, ptr)");
        emitModule("declare i1 @nexus_slist_remove(i64, i64, ptr)");
        emitModule("declare i1 @nexus_slist_try_pop(i64, ptr)");

        emitModule("declare i1 @nexus_gfx_init(i64, i64, ptr)");

        emitModule("declare ptr @nexus_gfx_error()");

        emitModule("declare void @nexus_gfx_shutdown()");

        emitModule("declare i1 @nexus_gfx_should_close()");

        emitModule("declare void @nexus_gfx_poll()");

        emitModule("declare i64 @nexus_gfx_event_type()");

        emitModule("declare i64 @nexus_gfx_event_key()");

        emitModule("declare i64 @nexus_gfx_event_mouse_button()");

        emitModule("declare i64 @nexus_gfx_event_x()");

        emitModule("declare i64 @nexus_gfx_event_y()");

        emitModule("declare i1 @nexus_gfx_event_is(ptr)");

        emitModule("declare i64 @nexus_gfx_width()");

        emitModule("declare i64 @nexus_gfx_height()");

        emitModule("declare void @nexus_gfx_set_ui_resolution(double,double)");

        emitModule("declare i1 @nexus_gfx_vsync(i1)");

        emitModule("declare double @nexus_gfx_mouse_x()");

        emitModule("declare double @nexus_gfx_mouse_y()");

        emitModule("declare i1 @nexus_gfx_mouse_down(i64)");

        emitModule("declare i1 @nexus_gfx_mouse_clicked(i64)");

        emitModule("declare i1 @nexus_gfx_key_down(ptr)");

        emitModule("declare i1 @nexus_gfx_key_pressed(ptr)");

        emitModule("declare double @nexus_gfx_dt()");

        emitModule("declare void @nexus_gfx_begin()");

        emitModule("declare void @nexus_gfx_end()");

        emitModule("declare void @nexus_gfx_clear(double,double,double,double)");

        emitModule("declare void "
                   "@nexus_gfx_rect(double,double,double,double,double,double,double,double)");

        emitModule(
            "declare void @nexus_gfx_circle(double,double,double,double,double,double,double)");

        emitModule(
            "declare void "
            "@nexus_gfx_line(double,double,double,double,double,double,double,double,double)");

        emitModule("declare void "
                   "@nexus_gfx_text(double,double,double,ptr,double,double,double,double)");

        emitModule("declare void @nexus_gfx_set_title(ptr)");

        emitModule("declare void @nexus_gfx_fullscreen(i1)");
        emitModule("declare void @nexus_gfx_maximize()");
        emitModule("declare void @nexus_gfx_restore()");
        emitModule("declare i1 @nexus_gfx_is_fullscreen()");
        emitModule("declare ptr @nexus_gfx_backend()");

        emitModule("declare i64 @nexus_gfx_texture_load(ptr)");

        emitModule("declare void "
                   "@nexus_gfx_texture_draw(i64,double,double,double,double,double,double,"
                   "double,double)");

        emitModule("declare void @nexus_gfx_texture_unload(i64)");

        emitModule("declare i64 @nexus_gfx_texture_width(i64)");

        emitModule("declare i64 @nexus_gfx_texture_height(i64)");

        emitModule("declare void "
                   "@nexus_gfx3d_begin(double,double,double,double,double,double,double,double,"
                   "double)");

        emitModule("declare void "
                   "@nexus_gfx3d_cube(double,double,double,double,double,double,double,double,"
                   "double,double)");

        emitModule("declare void @nexus_gfx3d_grid(i64,double,double,double,double,double)");

        emitModule("declare void @nexus_gfx3d_end()");

        emitModule("declare i64 @nexus_gfx_sound_load(ptr)");

        emitModule("declare void @nexus_gfx_sound_play(i64,i1)");

        emitModule("declare void @nexus_gfx_sound_stop()");

        emitModule("declare void @nexus_gfx_sound_unload(i64)");

        emitModule("declare i64 @nexus_gfx_audio_available()");

        emitModule("declare ptr @nexus_gfx_audio_error()");

        emitModule("declare void @nexus_gfx_sound_volume(double)");

        emitModule("declare i1 @nexus_gfx_sound_playing()");

        emitModule("declare void @nexus_gfx_sound_tone(double,i64,double)");

        emitModule("declare double @nexus_sqrt(double)");

        emitModule("declare double @nexus_sin(double)");
        emitModule("declare double @nexus_cos(double)");
        emitModule("declare double @nexus_tan(double)");
        emitModule("declare double @nexus_atan2(double, double)");
        emitModule("declare double @nexus_floor(double)");
        emitModule("declare double @nexus_ceil(double)");
        emitModule("declare double @nexus_pow(double, double)");

        emitModule("declare double @nexus_to_f64(i64)");

        emitModule("declare i64 @nexus_to_i64(double)");

        emitModule("declare i64 @nexus_gfx_canvas_create(i64,i64,double,double,double,double)");

        emitModule("declare void @nexus_gfx_canvas_destroy(i64)");

        emitModule("declare void @nexus_gfx_canvas_clear(i64,double,double,double,double)");

        emitModule(
            "declare void "
            "@nexus_gfx_canvas_brush(i64,double,double,double,double,double,double,double)");

        emitModule("declare void "
                   "@nexus_gfx_canvas_line(i64,double,double,double,double,double,double,"
                   "double,double,double)");

        emitModule("declare void "
                   "@nexus_gfx_canvas_rect(i64,double,double,double,double,double,double,"
                   "double,double,double,i1)");

        emitModule(
            "declare void @nexus_gfx_canvas_fill(i64,i64,i64,double,double,double,double)");

        emitModule(
            "declare void @nexus_gfx_canvas_draw(i64,double,double,double,double,double)");

        emitModule("declare i1 @nexus_gfx_canvas_save_bmp(i64,ptr)");

        emitModule("declare i64 @nexus_gfx_canvas_load_bmp(ptr)");

        emitModule("declare void @nexus_gfx_canvas_checkpoint(i64)");

        emitModule("declare i1 @nexus_gfx_canvas_undo(i64)");

        emitModule("declare i1 @nexus_gfx_canvas_redo(i64)");

        emitModule("declare i64 @nexus_gfx_canvas_width(i64)");

        emitModule("declare i64 @nexus_gfx_canvas_height(i64)");

        emitModule("declare i64 @nexus_gfx3d_model_load(ptr)");

        emitModule("declare void "
                   "@nexus_gfx3d_model_draw(i64,double,double,double,double,double,double,"
                   "double,double,double)");

        emitModule("declare void @nexus_gfx3d_model_unload(i64)");

        emitModule("declare i64 "
                   "@nexus_gfx_particle_create(double,double,double,double,double,double,"
                   "double,double,double,double)");

        emitModule("declare i1 @nexus_gfx_particle_alive(i64)");

        emitModule("declare void @nexus_gfx_particles_update(double)");

        emitModule("declare void @nexus_gfx_particles_draw()");

        emitModule("declare void @nexus_gfx_particle_destroy(i64)");

        emitModule("declare void @nexus_gfx_particles_clear()");

        emitModule("declare void @nexus_gfx_camera_set(double,double,double)");

        emitModule("declare void @nexus_gfx_camera_reset()");

        emitModule("declare void "
                   "@nexus_gfx_texture_draw_frame(i64,double,double,double,double,i64,i64,i64,"
                   "i64,double,double,double,double)");

        emitModule("declare ptr @nexus_gfx_image_type()");

        emitModule("declare i64 @nexus_gfx_font_load(ptr,i64)");

        emitModule("declare void "
                   "@nexus_gfx_text_font(i64,double,double,ptr,double,double,double,double)");

        emitModule("declare void @nexus_gfx_font_unload(i64)");
        emitModule("declare i64 @nexus_gfx_font_default(i64)");
        emitModule("declare i64 @nexus_gfx_text_width(i64, ptr)");
        emitModule("declare i64 @nexus_gfx_text_height(i64)");
        emitModule("declare void @nexus_gfx_font_style(i64, i64)");

        emitModule("declare i64 @nexus_gfx_music_load(ptr)");

        emitModule("declare i1 @nexus_gfx_music_play(i64,i1)");

        emitModule("declare void @nexus_gfx_music_stop()");

        emitModule("declare void @nexus_gfx_music_volume(double)");

        emitModule("declare void @nexus_gfx_cursor_visible(i1)");

        emitModule("declare i1 @nexus_ui_button(double,double,double,double,ptr)");
        emitModule("declare ptr @nexus_ui_text_field(double,double,double,double,ptr,ptr,i64)");

        emitModule("declare void "
                   "@nexus_ui_panel(double,double,double,double,double,double,double,double)");

        emitModule("declare void @nexus_ui_label(double,double,ptr,double)");

        emitModule(
            "declare void "
            "@nexus_ui_progress(double,double,double,double,double,double,double,double)");

        emitModule("declare i64 @nexus_ecs_create()");

        emitModule("declare void @nexus_ecs_destroy(i64)");

        emitModule("declare i1 @nexus_ecs_alive(i64)");

        emitModule("declare void @nexus_ecs_set_position(i64,double,double)");

        emitModule("declare void @nexus_ecs_set_velocity(i64,double,double)");

        emitModule("declare void @nexus_ecs_set_size(i64,double,double)");

        emitModule("declare void @nexus_ecs_set_color(i64,double,double,double,double)");

        emitModule("declare void @nexus_ecs_set_texture(i64,i64)");

        emitModule("declare void @nexus_ecs_update(double)");

        emitModule("declare void @nexus_ecs_draw()");

        emitModule("declare i1 @nexus_ecs_collides(i64,i64)");

        emitModule("declare double @nexus_ecs_x(i64)");

        emitModule("declare double @nexus_ecs_y(i64)");

        emitModule("declare i64 @nexus_ecs_count()");

        emitModule("declare void @nexus_gfx_texture_filter(i64,i1)");

        emitModule("declare i64 @nexus_gfx_anim_create(i64,i64,i64,i64,i64,double,i1)");

        emitModule("declare void @nexus_gfx_anim_update(i64,double)");

        emitModule("declare void "
                   "@nexus_gfx_anim_draw(i64,double,double,double,double,double,double,double,"
                   "double,double)");

        emitModule("declare i64 @nexus_gfx_anim_frame(i64)");

        emitModule("declare void @nexus_gfx_anim_destroy(i64)");

        emitModule(
            "declare i1 "
            "@nexus_physics_aabb(double,double,double,double,double,double,double,double)");

        emitModule(
            "declare i1 @nexus_physics_circle(double,double,double,double,double,double)");

        emitModule("declare i64 @nexus_scene_create()");

        emitModule("declare void @nexus_scene_add(i64,i64)");

        emitModule("declare i1 @nexus_scene_save(i64,ptr)");

        emitModule("declare i64 @nexus_scene_load(ptr)");

        emitModule("declare void @nexus_scene_destroy(i64)");

        emitModule("declare void @nexus_scene_clear(i64)");

        emitModule("declare i1 @nexus_ui_checkbox(double,double,ptr,i1)");

        emitModule("declare double @nexus_ui_slider(double,double,double,double,double)");

        for (auto& f : const_cast<Program&>(p).functions) {
            if (f.external) {
                std::ostringstream s;
                s << "declare " << llvmType(f.ret) << " @" << f.name << "(";
                for (size_t i = 0; i < f.params.size(); ++i) {
                    if (i)
                        s << ", ";
                    s << llvmType(f.params[i].type);
                }
                s << ")";
                emitModule(s.str());
                continue;
            }
            vars.clear();
            types.clear();
            reg = 0;
            label = 0;
            terminated = false;
            breakLabels.clear();
            continueLabels.clear();
            deferredCalls.clear();
            loopDeferScopeDepth.clear();
            body.str("");
            body.clear();
            Type codegenRet = f.ret;
            if (f.name == "main" && codegenRet.kind == TypeKind::Void)
                codegenRet = Type::i64();
            currentReturnType = codegenRet;
            std::ostringstream sig;
            sig << "define " << llvmType(codegenRet) << " @" << f.name << "(";
            for (size_t i = 0; i < f.params.size(); ++i) {
                if (i)
                    sig << ", ";
                sig << llvmType(f.params[i].type) << " %arg" << i;
            }
            sig << ") {";
            emit(sig.str());
            startBlock("entry");
            for (size_t i = 0; i < f.params.size(); ++i) {
                auto ptr = next();
                vars[f.params[i].name] = ptr;
                types[f.params[i].name] = f.params[i].type;
                emit(ptr + " = alloca " + llvmType(f.params[i].type));
                emit("store " + llvmType(f.params[i].type) + " %arg" + std::to_string(i) +
                     ", ptr " + ptr);
            }
            stmtBlock(f.body.get(), codegenRet);
            if (!terminated) {
                if (codegenRet.kind == TypeKind::Void)
                    emit("ret void");
                else if (f.name == "main")
                    emit("ret " + llvmType(codegenRet) + " 0");
                else
                    throw std::runtime_error("function may fall through without return: " +
                                             f.name);
            }
            emit("}");
            functionsIR << body.str();
        }
        std::ostringstream result;
        result << module.str();
        for (auto& g : globals)
            result << g << '\n';
        result << functionsIR.str();
        return result.str();
    }

} // namespace nexus
