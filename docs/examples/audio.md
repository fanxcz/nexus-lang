# Audio Example

The current audio demo is `examples/audio_demo.nx`. It is a small console-driven test of the optional runtime audio backend, not a graphical music player.

## Run

From the repository root:

```bash
nexus check examples/audio_demo.nx
nexus run examples/audio_demo.nx
```

The example checks `gfx_audio_available()`, loads `examples/assets/beep.wav`, sets the volume, plays the sample, waits briefly, checks playback state, and stops/unloads it. It prints any error from `gfx_audio_error()`.

## Functions used

- `gfx_audio_available()` and `gfx_audio_error()`
- `gfx_sound_load(path)`, `gfx_sound_play(id, loop)`
- `gfx_sound_volume(value)`, `gfx_sound_playing()`
- `gfx_sound_stop()`, `gfx_sound_unload(id)`
- `sleep(milliseconds)`

Audio support is dynamically loaded and may be unavailable on a host without the optional multimedia runtime libraries or a working audio device. Keep the repository root as the working directory, because the example uses a relative asset path. There is no separate `audio_init` or `audio_quit` runtime call.
