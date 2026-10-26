# `std.audio2d`

Audio wrappers use the optional runtime audio backend:

- `audio2d_load(path) -> i64`: returns 0 if loading fails.
- `audio2d_play(sound, looped)`: no-op for a non-positive handle.

## Supported files

The built-in fallback decoder accepts RIFF/WAVE files with PCM 8-bit or 16-bit
samples, mono or stereo, at any sample rate. Compressed formats such as MP3 and
Ogg require the optional `SDL2_mixer` runtime library; without it, keep WAV
assets in the repository.

Loaded samples are converted to the format the active audio device actually
opened with (unsigned 8-bit, signed 16-bit or 32-bit float) and resampled when
the device rate differs from the file rate. This matters on Windows hosts whose
default device reports `AUDIO_F32`; older releases rejected that format and
`audio2d_load` returned 0 with `failed to convert WAV to the active audio
device format`.

## Diagnostics

- `gfx_audio_available() -> i64` reports whether an audio device is open.
- `gfx_audio_error() -> string` returns the last load/playback error.
- `gfx_sound_unload(id)`, `gfx_sound_volume(value)`, `gfx_sound_playing()` and
  `gfx_sound_tone(frequency, duration_ms, volume)` are available as lower-level
  builtins.

Audio availability depends on the host libraries and a working output device. A
successfully compiled program may still run silently on a host without audio
support; check the loaded handle and `gfx_audio_error()` where relevant.
