#include "format/formatter.hpp"
#include "lexer/lexer.hpp"
#include "native/native.hpp"
#include "package/sha256.hpp"
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
            auto p = parse(
                "fn main(){ let n: i64 = 42; match n { 1..10 => { print(1) } "
                "value if value > 100 => { print(value) } _ => { print(0) } } }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p = parse(
                "fn main(){ let values = [1, 2, 3]; let mut total = 0; "
                "for value in values { total = total + value } print(total) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p = parse(
                "fn main(){ let text = \"nexus\"; print(string.len(text)); "
                "print(string.chars(text)); print(string.char_at(text, 0)); "
                "print(string.from_char(65)); print(string.valid_utf8(text)) }");

            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ for ch in \"abc\" { print(ch) } }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 6;
        }
        {
            bool failed = false;
            try {
                auto p = parse(
                    "fn main(){ let values = [1, 2]; for value in values { print(value) } "
                    "print(value) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 7;
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ let n = 1; match n { _ => { print(1) } 1 => { print(2) } "
                               "} }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 4;
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ let n = 1; match n { 1 if 2 => { print(1) } } }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 5;
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
        {
            const std::string input = "fn main(){\n"
                                      "let text=\"{\"; // }\n"
                                      "if true {\n"
                                      "print(text)  \n"
                                      "}\n"
                                      "}\n";
            const std::string expected = "fn main() {\n"
                                         "    let text = \"{\"; // }\n"
                                         "    if true {\n"
                                         "        print(text)\n"
                                         "    }\n"
                                         "}\n";
            const auto actual = nexus::formatNexusSource(input);
            if (actual != expected) {
                std::cerr << "formatter string/comment/indent regression\n"
                          << "Expected:\n"
                          << expected << "Actual:\n"
                          << actual;
                return 20;
            }
        }
        {
            const std::string input = "fn sum(a:i64,b:i64)->i64{\n"
                                      "return a+b\n"
                                      "}\n";
            const std::string expected = "fn sum(a: i64, b: i64) -> i64 {\n"
                                         "    return a + b\n"
                                         "}\n";
            const auto actual = nexus::formatNexusSource(input);
            if (actual != expected) {
                std::cerr << "formatter token-spacing regression\n"
                          << "Expected:\n"
                          << expected << "Actual:\n"
                          << actual;
                return 21;
            }
        }
        {
            bool reportedMultiple = false;
            try {
                auto invalid = parse("fn broken( { }\n"
                                     "fn also_broken( { }\n"
                                     "fn main() { }\n");
                (void)invalid;
            } catch (const std::exception& error) {
                const std::string message = error.what();
                reportedMultiple =
                    message.find("NEXUS found 2 syntax errors") != std::string::npos &&
                    message.find("expected identifier") != std::string::npos;
            }
            if (!reportedMultiple) {
                std::cerr << "parser multi-error recovery regression\n";
                return 22;
            }
        }
        {
            const std::string input = "fn main() { print(1) }\n";
            const std::string expected = "fn main() { print(1) }\n";
            if (nexus::formatNexusSource(input) != expected) {
                std::cerr << "formatter compact-block regression\n";
                return 23;
            }
        }
        {
            if (nexus::package::sha256Hex("") !=
                    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" ||
                nexus::package::sha256Hex("abc") !=
                    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
                std::cerr << "SHA-256 regression\\n";
                return 24;
            }
        }
        {
            const auto program = parse("fn main() { let value = 20 + 22; print(value) }");
            if (program.functions.empty() || !program.functions.front().span.valid ||
                program.functions.front().span.start.offset != 0 ||
                program.functions.front().span.end.offset == 0 ||
                program.functions.front().body->statements.empty() ||
                !program.functions.front().body->statements.front()->span.valid) {
                std::cerr << "AST source span regression\\n";
                return 25;
            }
            const auto* binding = dynamic_cast<const nexus::LetStmt*>(
                program.functions.front().body->statements.front().get());
            if (!binding || !binding->init || !binding->init->span.valid ||
                binding->init->span.end.offset <= binding->init->span.start.offset) {
                std::cerr << "AST expression span regression\\n";
                return 26;
            }
        }
        {
            // Source spans must use raw source positions, not decoded string-value length.
            const std::string spanSource = "fn main() { let value = \"x\\t\"; }";
            const auto program = parse(spanSource.c_str());
            const auto* binding = dynamic_cast<const nexus::LetStmt*>(
                program.functions.front().body->statements.front().get());
            const auto* literal =
                binding ? dynamic_cast<const nexus::StringExpr*>(binding->init.get()) : nullptr;
            const auto literalStart = spanSource.find("\"x\\t\"");
            if (!literal || literalStart == std::string::npos || !literal->span.valid ||
                literal->span.start.offset != literalStart ||
                literal->span.end.offset != literalStart + 5) {
                std::cerr << "escaped-string source span regression\n";
                return 27;
            }
        }
        {
            // Unicode scalar escapes decode to UTF-8, while source spans still
            // refer to the raw source bytes, not the decoded string length.
            const std::string unicodeSource =
                "fn main() { let value = \"A\\u{1F642}Z\"; }";
            const auto program = parse(unicodeSource.c_str());
            const auto* binding = dynamic_cast<const nexus::LetStmt*>(
                program.functions.front().body->statements.front().get());
            const auto* literal =
                binding ? dynamic_cast<const nexus::StringExpr*>(binding->init.get()) : nullptr;
            const std::string expected = std::string("A") + "\xF0\x9F\x99\x82" + "Z";
            const auto literalStart = unicodeSource.find("\"A\\u{1F642}Z\"");
            if (!literal || literal->value != expected || !literal->span.valid ||
                literalStart == std::string::npos ||
                literal->span.start.offset != literalStart ||
                literal->span.end.offset != literalStart + 13) {
                std::cerr << "Unicode string escape decoding/span regression\n";
                return 28;
            }

            const std::vector<std::string> invalidSources = {
                "fn main() { print(\"\\u{}\") }",
                "fn main() { print(\"\\u{D800}\") }",
                "fn main() { print(\"\\u{110000}\") }",
                "fn main() { print(\"\\u{0}\") }",
                "fn main() { print(\"\\u{1234567}\") }",
                "fn main() { print(\"\\u{ZZ}\") }",
                "fn main() { print(\"\\u1234\") }",
            };
            for (const auto& invalidSource : invalidSources) {
                try {
                    const auto invalidProgram = parse(invalidSource.c_str());
                    (void)invalidProgram;
                    std::cerr << "invalid Unicode escape unexpectedly parsed\n";
                    return 29;
                } catch (const std::exception& error) {
                    const std::string message = error.what();
                    if (message.find("Unicode escape") == std::string::npos &&
                        message.find("Unicode scalar value") == std::string::npos &&
                        message.find("U+0000") == std::string::npos) {
                        std::cerr << "unexpected Unicode escape diagnostic: " << message << "\n";
                        return 30;
                    }
                }
            }
        }
        std::cout << "Nexus tests: PASS\n";

        return 0;

    } catch (const std::exception& e) {

        std::cerr << e.what() << "\n";

        return 1;
    }
}
