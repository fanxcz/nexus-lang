#include "format/formatter.hpp"
#include "lexer/lexer.hpp"
#include "native/native.hpp"
#include "package/sha256.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static nexus::Program parse(const char* source, const std::string& sourcePath = {}) {
    return nexus::Parser(nexus::Lexer(source).tokenize(), sourcePath).parseProgram();
}

int main() {

    try {

        {
            auto p = parse("fn inspect(values: []i32) -> []i32 { return values } fn main() {}");
            if (p.functions.size() != 2 || p.functions[0].params.size() != 1 ||
                p.functions[0].params[0].type.kind != nexus::TypeKind::Slice ||
                !p.functions[0].params[0].type.element ||
                p.functions[0].params[0].type.element->kind != nexus::TypeKind::I32 ||
                p.functions[0].ret.kind != nexus::TypeKind::Slice ||
                !nexus::sameType(p.functions[0].params[0].type,
                                 nexus::Type::slice(nexus::Type::i32()))) {
                std::cerr << "slice type parsing or equality failed\n";
                return 74;
            }
            auto nested = parse("fn nested(value: [][]u16) {} fn main() {}");
            if (nested.functions[0].params[0].type.kind != nexus::TypeKind::Slice ||
                !nested.functions[0].params[0].type.element ||
                nested.functions[0].params[0].type.element->kind != nexus::TypeKind::Slice) {
                std::cerr << "nested slice type parsing failed\n";
                return 75;
            }
            bool sliceSignaturesAccepted = true;
            try {
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& error) {
                sliceSignaturesAccepted = false;
                std::cerr << "slice function signatures failed semantic validation: "
                          << error.what() << "\n";
            }
            if (!sliceSignaturesAccepted) {
                std::cerr << "direct slice parameters and return types should be supported\n";
                return 76;
            }
        }

        {
            auto p = parse("fn main(){ let values = [10, 20, 30, 40]; "
                           "let middle = values[1 + 0..3 - 0]; }");
            if (p.functions.size() != 1 || !p.functions[0].body ||
                p.functions[0].body->statements.size() != 2) {
                std::cerr << "slice expression did not parse into the function body\n";
                return 77;
            }
            const auto* binding =
                dynamic_cast<const nexus::LetStmt*>(p.functions[0].body->statements[1].get());
            const auto* slice = binding ? dynamic_cast<const nexus::SliceExpr*>(binding->init.get())
                                        : nullptr;
            if (!slice || !dynamic_cast<const nexus::VarExpr*>(slice->object.get()) ||
                !dynamic_cast<const nexus::BinaryExpr*>(slice->start.get()) ||
                !dynamic_cast<const nexus::BinaryExpr*>(slice->end.get()) ||
                !slice->span.valid || !slice->object->span.valid ||
                !slice->start->span.valid || !slice->end->span.valid) {
                std::cerr << "slice expression AST shape or source spans are incorrect\n";
                return 78;
            }

            auto chained = parse(
                "fn main(){ let values = [10, 20, 30, 40]; let item = values[1..3][0]; }");
            const auto* chainedBinding = dynamic_cast<const nexus::LetStmt*>(
                chained.functions[0].body->statements[1].get());
            const auto* index = chainedBinding
                                    ? dynamic_cast<const nexus::IndexExpr*>(
                                          chainedBinding->init.get())
                                    : nullptr;
            if (!index || !dynamic_cast<const nexus::SliceExpr*>(index->object.get())) {
                std::cerr << "postfix indexing after slicing did not parse\n";
                return 79;
            }

            const char* malformedSlices[] = {
                "fn main(){ let values = [1, 2]; let bad = values[..2]; }",
                "fn main(){ let values = [1, 2]; let bad = values[1..]; }",
                "fn main(){ let values = [1, 2]; let bad = values[1..2; }",
            };
            for (const auto* source : malformedSlices) {
                bool rejected = false;
                try {
                    (void)parse(source);
                } catch (const std::exception&) {
                    rejected = true;
                }
                if (!rejected) {
                    std::cerr << "malformed slice expression was accepted: " << source << "\n";
                    return 80;
                }
            }

            bool sliceExpressionAccepted = true;
            try {
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& error) {
                sliceExpressionAccepted = false;
                std::cerr << "slice expression failed semantic validation: " << error.what()
                          << "\n";
            }
            if (!sliceExpressionAccepted) {
                std::cerr << "slice expression should pass semantic validation\n";
                return 81;
            }
        }

        {
            const auto analyzeError = [](const char* source) {
                try {
                    auto program = parse(source);
                    nexus::SemanticAnalyzer().analyze(program);
                } catch (const std::exception& error) {
                    return std::string(error.what());
                }
                return std::string{};
            };

            const auto valid = analyzeError(
                "fn main(){ let values = [10, 20, 30, 40]; "
                "let middle = values[1 + 0..3 - 0]; let item = middle[0]; "
                "let nested = values[1..4][0..2]; let empty = values[4..4]; }");
            if (!valid.empty() ||
                valid.find("slice start bound") != std::string::npos ||
                valid.find("slice end bound") != std::string::npos ||
                valid.find("indexing requires") != std::string::npos) {
                std::cerr << "valid slice expressions failed semantic type checking: " << valid
                          << "\n";
                return 82;
            }

            const auto dynamicBounds = analyzeError(
                "fn main(){ let values = [10,20,30,40]; let start: i64 = 1; "
                "let end: i64 = 3; let view = values[start..end]; }");
            if (!dynamicBounds.empty() ||
                dynamicBounds.find("slice start bound") != std::string::npos ||
                dynamicBounds.find("slice end bound") != std::string::npos) {
                std::cerr << "well-typed dynamic bounds were rejected: " << dynamicBounds << "\n";
                return 87;
            }

            const struct {
                const char* source;
                const char* expected;
                const char* label;
            } invalidRanges[] = {
                {"fn main(){ let values = [1,2]; let s = values[-1..2]; }",
                 "slice start bound is out of bounds", "negative start"},
                {"fn main(){ let values = [1,2]; let s = values[0..3]; }",
                 "slice end bound is out of bounds", "end beyond array"},
                {"fn main(){ let values = [1,2]; let s = values[2..1]; }",
                 "slice start bound exceeds end bound", "reversed range"},
                {"fn main(){ let values = [1,2]; let s = values[1.0..2]; }",
                 "slice start bound must be i64", "non-integer start"},
                {"fn main(){ let value = 7; let s = value[0..1]; }",
                 "slice expression requires an array or slice", "non-array source"},
                {"fn make() -> [i64; 2] { return [1,2]; } "
                 "fn main(){ let s = make()[0..1]; }",
                 "slice source must be a named array or slice binding", "temporary source"},
            };
            for (const auto& test : invalidRanges) {
                const auto error = analyzeError(test.source);
                if (error.find(test.expected) == std::string::npos) {
                    std::cerr << "slice semantic test failed (" << test.label << "): " << error
                              << "\n";
                    return 83;
                }
            }

            const auto localEscape = analyzeError(
                "fn escape() -> []i64 { let values = [10, 20]; return values[0..2]; }");
            if (localEscape.find("cannot return a pointer to local storage 'values'") ==
                std::string::npos) {
                std::cerr << "slice of local array escaped its lexical lifetime: " << localEscape
                          << "\n";
                return 84;
            }

            const auto aliasedLocalEscape = analyzeError(
                "fn escape() -> []i64 { let values = [10, 20]; "
                "let view = values[0..2]; return view; }");
            if (aliasedLocalEscape.find("cannot return a pointer to local storage 'values'") ==
                std::string::npos) {
                std::cerr << "slice alias lost the origin of its local array: "
                          << aliasedLocalEscape << "\n";
                return 86;
            }

            const auto parameterBorrow = analyzeError(
                "fn passthrough(values: []i32) -> []i32 { return values; } fn main() {}");
            if (parameterBorrow.find("cannot return a pointer to local storage") !=
                std::string::npos) {
                std::cerr << "slice borrowed from a parameter was incorrectly treated as local: "
                          << parameterBorrow << "\n";
                return 85;
            }

            const auto lenChecks = analyzeError(
                "fn main(){ let values = [10,20,30]; let view = values[1..3]; "
                "let count: i64 = len(view); let total: i64 = len(values); "
                "let text: string = \"hello\"; let chars: i64 = len(text); }");
            if (!lenChecks.empty()) {
                std::cerr << "len should accept arrays and slices: " << lenChecks << "\n";
                return 88;
            }
            const auto invalidLen = analyzeError("fn main(){ let count = len(42); }");
            if (invalidLen.find("len expects a string, array, or slice") == std::string::npos) {
                std::cerr << "len accepted a non-array value: " << invalidLen << "\n";
                return 89;
            }
            const auto mutableSliceWrite = analyzeError(
                "fn main(){ let mut values = [1,2,3]; let mut view = values[0..2]; view[0] = 9; }");
            if (mutableSliceWrite.find("index assignment requires a mutable array or mutable slice") ==
                std::string::npos) {
                std::cerr << "slice write should remain unsupported until mutable-slice rules exist: "
                          << mutableSliceWrite << "\n";
                return 90;
            }
        }

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
            auto p = parse(
                "fn set_value(out: *i64, value: i64) { *out = value } "
                "fn main(){ let mut value: i64 = 0; set_value(&value, 42); print(value) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            // Address-of creates a mutable pointer in the current type system.
            // Reject immutable roots to prevent mutation through an alias.
            const char* invalidSources[] = {
                "fn main(){ let value: i64 = 10; let ptr = &value; *ptr = 11 }",
                "struct Data { value: i64 } fn main(){ let data = Data{value: 10}; let ptr = &data.value }",
                "fn main(){ let values = [1, 2, 3]; let ptr = &values[0] }",
            };
            for (const auto* source : invalidSources) {
                bool rejected = false;
                try {
                    auto p = parse(source);
                    nexus::SemanticAnalyzer().analyze(p);
                } catch (const std::exception& error) {
                    rejected = std::string(error.what()).find(
                                   "'&' requires mutable storage") != std::string::npos;
                }
                if (!rejected) {
                    std::cerr << "address-of immutable storage was not rejected: "
                              << source << "\\n";
                    return 48;
                }
            }
        }
        {

                    {
            const char* invalidNestedAssignments[] = {
                "struct Inner { value: i64 } struct Outer { inner: Inner } "
                "fn main(){ let outer = Outer{inner: Inner{value: 10}}; outer.inner.value = 11 }",
                "struct Data { values: [i64; 3] } "
                "fn main(){ let data = Data{values: [1, 2, 3]}; data.values[0] = 99 }"
            };
            for (const auto* source : invalidNestedAssignments) {
                bool rejected = false;
                try {
                    auto p = parse(source);
                    nexus::SemanticAnalyzer().analyze(p);
                } catch (const std::exception& error) {
                    rejected = std::string(error.what()).find("immutable variable") !=
                               std::string::npos;
                }
                if (!rejected)
                    return 55;
            }
            auto mutableNested = parse(
                "struct Inner { value: i64 } struct Outer { inner: Inner } "
                "fn main(){ let mut outer = Outer{inner: Inner{value: 10}}; "
                "outer.inner.value = 11 }");
            nexus::SemanticAnalyzer().analyze(mutableNested);
        }
        {
            const char* invalidVoidSources[] = {
                "fn main(){ let result = print(\"not a value\") }",
                "fn consume(value: void) {} fn main() {}",
                "struct Bad { value: void } fn main() {}",
                "fn bad() -> *void {} fn main() {}",
                "fn bad() -> [void; 1] {} fn main() {}",
                "enum Bad { Empty(void) } fn main() {}",
            };
            for (const auto* source : invalidVoidSources) {
                bool rejected = false;
                try {
                    auto p = parse(source);
                    nexus::SemanticAnalyzer().analyze(p);
                } catch (const std::exception& error) {
                    rejected = std::string(error.what()).find("void") != std::string::npos;
                }
                if (!rejected) {
                    std::cerr << "invalid void value/type was accepted: " << source << "\\n";
                    return 56;
                }
            }
            auto validVoid = parse("fn helper() -> void { return } fn main(){ helper() }");
            nexus::SemanticAnalyzer().analyze(validVoid);
        }
        {
            bool sourcePathPreserved = false;
            try {
                auto p = parse("fn main(){ let result: i64 = \"bad\" }",
                               "/virtual/modules/library.nx");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& error) {
                sourcePathPreserved =
                    std::string(error.what()).find("/virtual/modules/library.nx:") !=
                    std::string::npos;
            }
            if (!sourcePathPreserved)
                return 57;
        }
        {
            bool rejectedAbi = false;
            try {
                auto p = parse("extern \"stdcall\" fn puts(value: string); fn main() {}");
                (void)p;
            } catch (const std::exception& error) {
                rejectedAbi = std::string(error.what()).find("unsupported extern ABI") !=
                              std::string::npos;
            }
            if (!rejectedAbi)
                return 58;
            auto cAbi = parse("extern \"C\" fn puts(value: string); fn main() {}");
            nexus::SemanticAnalyzer().analyze(cAbi);
        }
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
            auto p = parse(
                "fn main(){ let text = \"A\\u{00E9}\\u{1F642}\"; "
                "for codepoint in text { print(string.from_char(codepoint)) } }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ for item in 42 { print(item) } }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception&) {
                failed = true;
            }
            if (!failed)
                return 31;
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
            bool failed = false;
            try {
                auto p = parse("fn main(){ let a = [1, 2, 3]; print(a[-1]) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& error) {
                failed = std::string(error.what()).find("array index is out of bounds") !=
                         std::string::npos;
            }
            if (!failed)
                return 53;
        }
        {
            bool failed = false;
            try {
                auto p = parse("fn main(){ let mut a = [1, 2, 3]; a[-1] = 7 }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& error) {
                failed = std::string(error.what()).find("array index is out of bounds") !=
                         std::string::npos;
            }
            if (!failed)
                return 54;
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
            // A payload binding matches only its enum variant; it is not a catch-all.
            bool failed = false;
            try {
                auto p = parse(
                    "enum Maybe { Some(i64), None } "
                    "fn choose(value: Maybe) -> i64 { "
                    "match value { Maybe::Some(number) => { return number } } } "
                    "fn main(){ print(choose(Maybe::Some(3))) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed = std::string(e.what()).find(
                    "function may exit without returning a value: choose") != std::string::npos;
            }
            if (!failed)
                return 45;
        }
        {
            // A binding pattern without an explicit value pattern does cover all values.
            auto p = parse(
                "fn identity(value: i64) -> i64 { "
                "match value { item => { return item } } } "
                "fn main(){ print(identity(7)) }");
            nexus::SemanticAnalyzer().analyze(p);
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
            auto p = parse(
                "enum Payload { Number(i64), Real(f64), Flag(bool), Text(string), Empty } "
                "fn main(){ let value = Payload::Flag(true); "
                "match value { Payload::Flag(flag) => { print(flag) } _ => { print(false) } } }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed = false;
            try {
                auto p = parse("enum Payload { Number(i64), Empty } "
                               "fn main(){ let value = Payload::Number(\"bad\") }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed = std::string(e.what()).find(
                    "expects payload 'i64' but got 'string'") != std::string::npos;
            }
            if (!failed)
                return 41;
        }
        {
            bool failed = false;
            try {
                auto p = parse("enum Payload { Number(i64), Empty } "
                               "fn main(){ let value = Payload::Number }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed = std::string(e.what()).find("requires a payload") != std::string::npos;
            }
            if (!failed)
                return 42;
        }
        {
            bool failed = false;
            try {
                auto p = parse("enum Payload { Empty } fn main(){ let value = Payload::Empty(3) }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed = std::string(e.what()).find("does not accept a payload") != std::string::npos;
            }
            if (!failed)
                return 43;
        }
        {
            bool failed = false;
            try {
                auto p = parse("enum Result { Err(string), Ok(i64) } "
                               "fn wrong()->i64 { let number = Result::Ok(4)?; return number } "
                               "fn main() {}");
                nexus::SemanticAnalyzer().analyze(p);
            } catch (const std::exception& e) {
                failed = std::string(e.what()).find("enclosing function to return 'enum Result'") !=
                         std::string::npos;
            }
            if (!failed)
                return 44;
        }
        {
            // Constant integer division and remainder by zero must fail during
            // semantic analysis, rather than reaching a hardware exception.
            const char* invalidSources[] = {
                "fn main(){ let value = 10 / 0 }",
                "fn main(){ let value = 10 % 0 }",
                "fn main(){ let value = 10 / -0 }",
                "fn main(){ let value = 10 % -0 }",
            };
            for (const auto* source : invalidSources) {
                bool rejected = false;
                try {
                    auto p = parse(source);
                    nexus::SemanticAnalyzer().analyze(p);
                } catch (const std::exception& error) {
                    rejected = std::string(error.what()).find("division by zero") !=
                               std::string::npos;
                }
                if (!rejected) {
                    std::cerr << "constant division by zero was not diagnosed: " << source << "\n";
                    return 46;
                }
            }
        }
        {
            // Compound assignment must reject a constant zero divisor for
            // fields, array elements and pointer targets before code generation.
            const char* invalidSources[] = {
                "struct Counter { value: i64 } fn main(){ let mut c = Counter{value: 10}; c.value /= 0 }",
                "struct Counter { value: i64 } fn main(){ let mut c = Counter{value: 10}; c.value %= -0 }",
                "fn main(){ let mut values = [10, 20]; values[0] /= 0 }",
                "fn main(){ let mut values = [10, 20]; values[0] %= -0 }",
                "fn main(){ let mut value: i64 = 10; let ptr = &value; *ptr /= 0 }",
            };
            for (const auto* source : invalidSources) {
                bool rejected = false;
                try {
                    auto p = parse(source);
                    nexus::SemanticAnalyzer().analyze(p);
                } catch (const std::exception& error) {
                    rejected = std::string(error.what()).find("division by zero") !=
                               std::string::npos;
                }
                if (!rejected) {
                    std::cerr << "compound assignment division by zero was not diagnosed: "
                              << source << "\n";
                    return 47;
                }
            }
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
                    {
            // The native entry stub calls main without arguments and forwards RAX/EAX
            // to the operating-system exit API, so validate its signature.
            auto withArguments = parse("fn main(value: i64) {}");
            nexus::SemanticAnalyzer().analyze(withArguments);
            std::string entryReason;
            if (nexus::native::supports(withArguments, entryReason) ||
                entryReason.find("must not take parameters") == std::string::npos)
                return 59;

            auto stringResult = parse("fn main() -> string { return \"bad\" }");
            nexus::SemanticAnalyzer().analyze(stringResult);
            if (nexus::native::supports(stringResult, entryReason) ||
                entryReason.find("must return") == std::string::npos)
                return 60;
        }
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
