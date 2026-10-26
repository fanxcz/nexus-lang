# `std.audio2d`

Audio wrappers use the optional runtime audio backend:

- `audio2d_load(path) -> i64`: returns 0 if loading fails.
- `audio2d_play(sound, looped)`: no-op for a non-positive handle.

Audio availability depends on the host libraries and installed codecs. A successfully compiled program may still run on a host without audio support; check the loaded handle and use `gfx_error()` where relevant.
