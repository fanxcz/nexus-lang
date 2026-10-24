# Audio

Audio is exposed through the current `gfx_sound_*`, `gfx_music_*` and availability helpers. There is no separate `audio_init` / `audio_quit` API. Optional audio support is loaded dynamically and may be unavailable on a host.

## Check availability

```nx
print("Audio available: " + str_i64(gfx_audio_available()))
if gfx_audio_available() == 0 {
    print(gfx_audio_error())
}
```

`gfx_audio_available()` returns an integer status; nonzero means available. `gfx_audio_error()` returns the current error string.

## Sound

```nx
let sound = gfx_sound_load("examples/assets/beep.wav")
if sound == 0 {
    print("Could not load sound: " + gfx_audio_error())
    return
}

gfx_sound_volume(1.0)
gfx_sound_play(sound, false)
sleep(400)
print("Playing: " + str_bool(gfx_sound_playing()))
gfx_sound_stop()
gfx_sound_unload(sound)
```

Functions include `gfx_sound_load(path)`, `gfx_sound_play(id, loop)`, `gfx_sound_stop()`, `gfx_sound_unload(id)`, `gfx_sound_volume(value)`, `gfx_sound_playing()` and `gfx_sound_tone(frequencyHz, durationMs, volume)`.

## Music

```nx
let music = gfx_music_load("assets/theme.ogg")
if music != 0 {
    gfx_music_volume(0.45)
    gfx_music_play(music, true)
    gfx_music_stop()
}
```

Music helpers are `gfx_music_load`, `gfx_music_play`, `gfx_music_stop` and `gfx_music_volume`. The current API does not expose independent channels, per-effect volumes, pause/resume or channel reservation.

See [audio_demo.nx](https://github.com/fanxcz/nexus-lang/blob/main/examples/audio_demo.nx). The example uses a relative asset path and should be run from the repository root.
