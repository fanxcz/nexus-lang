#include "lexer/lexer.hpp"
#include "native/native.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static nexus::Program parse(const char* source) {

    return nexus::Parser(nexus::Lexer(source).tokenize()).parseProgram();
}

int main() {

    try {

        {

            auto p = parse("fn main(){ let mut x: i64 = 2 + 3; x = x + 1; print(x) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse(
                "struct Player { id: i64 health: i64 speed: f64 } fn main(){ let mut p = "
                "Player{id:1,health:100,speed:4.5}; p.health = p.health - 25; print(p.health) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse("fn inc(x: i64)->i64 { return x + 1 } fn main(){ print(inc(41)) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse("fn main(){ let name = \"Nexus\"; print(\"Hello, \" + name); print(name "
                           "== \"Nexus\"); print(name != \"Other\") }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse("enum Color { Red, Green, Blue } fn main(){ let c = Color::Green; match "
                           "c { Color::Red => { print(\"red\") } Color::Green => { "
                           "print(\"green\") } _ => { print(\"other\") } } }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p =
                parse("fn main(){ let mut a = [1, 2, 3]; a[1] = 99; let i: i64 = 2; print(a[i]) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse("fn main(){ const base: i64 = 2; let mut total: i64 = 0; for i in 1..6 "
                           "{ total += i * base } print(total) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            bool failed = false;

            try {
                auto p = parse("fn main(){ const x: i64 = 1; x = 2 }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 3;
        }
        {

            bool failed = false;

            try {

                auto p = parse("fn main(){ let x = 1; x = 2 }");

                nexus::SemanticAnalyzer().analyze(p);

            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 1;
        }
        {

            bool failed = false;

            try {
                auto p = parse("fn main(){ let a = [1, 2, 3]; print(a[3]) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 2;
        }
        {

            auto p =
                parse("fn main(){ let mut running = true; while running { running = false } let "
                      "name = input(\"Name: \"); let n = input_i64(\"N: \"); let x = str_i64(n); "
                      "let ok = file_write(\"/tmp/nexus-tests.txt\", x); print(ok) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse("fn main(){ let s=gfx_sound_load(\"examples/assets/beep.wav\"); let "
                           "a=gfx_audio_available(); let e=gfx_audio_error(); "
                           "gfx_sound_volume(0.5); gfx_sound_playing(); gfx_sound_play(s,false); "
                           "gfx_sound_stop(); gfx_sound_unload(s); }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse(
                "fn main(){ let m=gfx3d_model_load(\"examples/assets/cube.obj\"); "
                "gfx3d_model_draw(m,0.0,0.0,0.0,1.0,1.0,1.0,0.0,0.0,0.0); gfx3d_model_unload(m); "
                "let p=gfx_particle_create(0.0,0.0,1.0,1.0,1.0,4.0,1.0,0.0,0.0,1.0); "
                "gfx_particle_alive(p); gfx_particles_update(0.016); gfx_particles_draw(); "
                "gfx_particle_destroy(p); }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {

            auto p = parse(
                "fn main(){ let s=scene_create(); let e=ecs_create(); "
                "ecs_set_position(e,10.0,20.0); ecs_set_size(e,32.0,32.0); scene_add(s,e); "
                "scene_save(s,\"/tmp/nexus-scene.nxs\"); let "
                "s2=scene_load(\"/tmp/nexus-scene.nxs\"); scene_clear(s2); scene_destroy(s2); "
                "scene_destroy(s); let hit=physics_aabb(0.0,0.0,10.0,10.0,5.0,5.0,4.0,4.0); let "
                "circ=physics_circle(0.0,0.0,5.0,6.0,0.0,2.0); let "
                "a=gfx_anim_create(1,16,16,4,8,12.0,true); gfx_anim_update(a,0.1); "
                "gfx_anim_frame(a); gfx_anim_destroy(a); ui_checkbox(10.0,10.0,\"Ready\",true); "
                "ui_slider(10.0,50.0,200.0,20.0,0.5); }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p = parse("fn choose(flag: bool)->i64 { if flag { return 1 } else { return 2 } } "
                           "fn main(){ print(choose(true)) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn choose(flag: bool)->i64 { if flag { return 1 } } fn main(){ "
                               "print(choose(true)) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed =
                    std::string(e.what()).find(
                        "function may exit without returning a value: choose") != std::string::npos;
            }
            if (!failed)
                return 4;
        }
        {
            auto p = parse("fn choose(flag: bool)->i64 { if flag { return 1 } return 2 } fn "
                           "main(){ print(choose(false)) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed = false;
            try {
                auto p =
                    parse("fn consume(count: i64) { print(count) } fn main(){ consume(\"oops\") }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                const std::string message = e.what();
                failed = message.find(
                             "argument 1 ('count') of 'consume' expects 'i64' but got 'string'") !=
                         std::string::npos;
            }
            if (!failed)
                return 5;
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ let count: i64 = \"oops\" }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                const std::string message = e.what();
                failed =
                    message.find("cannot initialize 'count': expected 'i64' but got 'string'") !=
                    std::string::npos;
            }
            if (!failed)
                return 6;
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ file_exists(42) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                const std::string message = e.what();
                failed =
                    message.find(
                        "argument 1 of builtin 'file_exists' expects 'string' but got 'i64'") !=
                    std::string::npos;
            }
            if (!failed)
                return 7;
        }
        {
            // Builtin native backend: target selection.
            nexus::native::NativeTarget t = nexus::native::NativeTarget::LinuxX64;
            if (!nexus::native::resolveTarget("x86_64-windows", t) ||
                t != nexus::native::NativeTarget::WindowsX64)
                return 8;
            if (!nexus::native::resolveTarget("x86_64-linux", t) ||
                t != nexus::native::NativeTarget::LinuxX64)
                return 8;
            if (nexus::native::resolveTarget("aarch64-macos", t))
                return 8;
            if (nexus::native::resolveTarget("wasm32-wasi", t))
                return 8;
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__))
            if (!nexus::native::resolveTarget("native", t) ||
                t != nexus::native::NativeTarget::WindowsX64)
                return 8;
#elif defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
            if (!nexus::native::resolveTarget("native", t) ||
                t != nexus::native::NativeTarget::LinuxX64)
                return 8;
#else
            if (nexus::native::resolveTarget("native", t))
                return 8;
#endif
        }
        {
            // Builtin native backend: subset accepted, image is a real PE.
            auto p = parse(
                "fn add(a: i64, b: i64) -> i64 { return a + b } fn main(){ print(add(20, 22)) }");
            nexus::SemanticAnalyzer().analyze(p);
            std::string reason;
            if (!nexus::native::supports(p, reason))
                return 9;
            const auto image =
                nexus::native::buildExecutable(p, nexus::native::NativeTarget::WindowsX64);
            if (image.size() < 64 || image[0] != 'M' || image[1] != 'Z')
                return 9;
            const auto elf =
                nexus::native::buildExecutable(p, nexus::native::NativeTarget::LinuxX64);
            if (elf.size() < 64 || elf[0] != 0x7F || elf[1] != 'E' || elf[2] != 'L' ||
                elf[3] != 'F')
                return 9;
        }
        {
            // Builtin native backend: non-subset programs rejected precisely.
            auto p = parse("struct P { x: i64 } fn main(){ let v = P { x: 1 }; print(v.x) }");
            nexus::SemanticAnalyzer().analyze(p);
            std::string reason;
            if (nexus::native::supports(p, reason))
                return 10;
            if (reason.find("struct") == std::string::npos)
                return 10;
            auto q = parse("fn main(){ print(1.5) }");
            nexus::SemanticAnalyzer().analyze(q);
            if (nexus::native::supports(q, reason))
                return 10;
            // ...while strings are accepted.
            auto s = parse("fn main(){ let name = \"Nexus\"; print(\"Hello, \" + name); print(name "
                           "== \"Nexus\") }");
            nexus::SemanticAnalyzer().analyze(s);
            if (!nexus::native::supports(s, reason))
                return 10;
        }
        std::cout << "Nexus tests: PASS\n";

        return 0;

    } catch (const std::exception& e) {

        std::cerr << e.what() << "\n";

        return 1;
    }
}
