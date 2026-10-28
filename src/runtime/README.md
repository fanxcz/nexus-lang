# NEXUS runtime sources

The runtime is compiled as **one translation unit**. `runtime.cpp` contains only
the ordering of its parts: it `#include`s small `.inc` files in the sequence in
which they must appear. Keeping a single translation unit is deliberate — the
runtime holds a large amount of shared static state (SDL entry points, the
window/renderer handles, texture/font/canvas registries, audio mixers, the
managed-allocation table), and splitting it into separately compiled objects
would require exposing all of that state through headers.

The `.inc` files are not standalone translation units. They are ordinary C++
fragments that are textually included inside `namespace { ... }` or
`extern "C" { ... }` blocks opened by `runtime.cpp`, so they must be edited with
that context in mind (for example, they cannot add their own includes at the
top of the file).

## Layout

| Directory       | Contents                                                        |
| --------------- | --------------------------------------------------------------- |
| `core/`         | prelude includes, allocator/GC, args, random, shell, system       |
| `graphics/`     | SDL/GL declarations, bindings, lifecycle, 2D/3D, canvas, text     |
| `audio/`        | WAV decoding, PCM mixing, sound/music playback                    |
| `io/`           | console, input, file helpers                                      |
| `data/`         | strict JSON parser and API, SQLite bindings                       |
| `strings/`      | UTF-8 and string built-ins                                        |
| `collections/`  | list/array built-ins                                              |
| `ui/`           | immediate-mode widgets                                            |
| `game/`         | physics, scenes, ECS                                              |
| `math/`         | scalar helpers                                                    |
| `memory/`       | raw memory built-ins                                              |
| `net/`          | HTTP client helpers                                               |
| `crypto/`       | SHA-256                                                           |

`runtime.c` is the optional C fallback runtime used when a C compiler is
preferred over C++ for a target.

## Adding code

1. Pick the topic directory that matches the API being added.
2. Add the definition to the relevant `.inc` file (or create a new one).
3. If you created a file, add its `#include` to `runtime.cpp` in the correct
   block, preserving initialization order.
4. Rebuild and run `ctest -R runtime_` to exercise the runtime unit tests.
