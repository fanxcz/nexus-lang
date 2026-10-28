// NEXUS runtime: one translation unit assembled from topic files.
//
// The runtime keeps a large amount of shared static state (SDL entry
// points, window/renderer handles, texture and canvas registries), so it
// is intentionally compiled as a single translation unit. Each part lives
// in a small file under this directory and is textually included in order.
#include "core/prelude.inc"

namespace {
#include "graphics/sdl_types.inc"
#include "graphics/gl_types.inc"
#include "graphics/sdl_bindings.inc"
#include "graphics/state.inc"
#include "graphics/library_loader.inc"
#include "graphics/software2d.inc"
#include "graphics/opengl2d.inc"
#include "graphics/text.inc"
#include "graphics/events.inc"
#include "audio/wav.inc"
#include "audio/pcm.inc"
#include "graphics/lifecycle.inc"
} // namespace

namespace {
#include "core/shell.inc"
#include "data/sqlite.inc"
} // namespace

namespace {
#include "core/memory.inc"
#include "data/json.inc"
} // namespace

extern "C" {
#include "math/scalar.inc"
#include "core/alloc.inc"
#include "io/input.inc"
#include "core/random.inc"
#include "core/system.inc"
#include "io/files.inc"
#include "memory/raw.inc"
#include "net/http.inc"
#include "data/json_api.inc"
#include "data/sqlite_api.inc"
#include "crypto/sha256.inc"
} // extern "C"

#include "core/args.inc"

extern "C" {
#include "core/os_args.inc"
#include "strings/utf8.inc"
#include "collections/lists.inc"
#include "io/console.inc"
#include "graphics/gfx_core.inc"
#include "graphics/gfx_draw.inc"
#include "graphics/gfx_primitives.inc"
#include "graphics/gfx_textures.inc"
#include "graphics/gfx_animation.inc"
#include "graphics/canvas.inc"
#include "graphics/canvas_api.inc"
#include "ui/widgets.inc"
#include "game/physics.inc"
#include "game/scene.inc"
#include "game/ecs.inc"
#include "graphics/gfx3d.inc"
#include "graphics/particles.inc"
#include "audio/mixer.inc"
} // extern "C"
