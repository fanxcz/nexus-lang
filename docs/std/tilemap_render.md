# `std.tilemap_render`

This module contains the tilemap debug renderer and depends on `std.tilemap`.

`tilemap_render_draw(map, map_width, map_height, tile_size, view_x, view_y, view_width, view_height)` draws only cells intersecting the viewport. The viewport is in world coordinates, while drawing positions are shifted so its upper-left corner becomes screen coordinate (0, 0).

The built-in colored renderer uses ID 1 for green/grass, 2 for brown/dirt, 3 for gray/stone, 4 for red and blue for other positive IDs. Tile ID 0 is not drawn.

```nx
import std.tilemap
import std.tilemap_render

fn main() {
    let map = tilemap_create(20, 12)
    tilemap_set(map, 20, 12, 3, 8, 1)
    gfx_begin()
    tilemap_render_draw(map, 20, 12, 32.0, 0.0, 0.0, 800.0, 480.0)
    gfx_end()
}
```

Call from inside the runtime's drawing frame. This basic renderer draws colored rectangles only; it does not load Tiled JSON, texture atlases or animated tile assets.
