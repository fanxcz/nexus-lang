# `std.audio2d`

Optional audio helpers for games and interactive apps.

- audio2d_load(path) -> i64: returns 0 if loading fails.
- audio2d_play(sound, looped): plays a loaded sample.
- audio2d_unload(sound): releases a positive sound handle.
- audio2d_stop(): stops active sound playback.
- audio2d_set_volume(volume): clamps master volume to [0, 1].
- audio2d_is_playing() -> bool: reports whether sound playback is active.
- audio2d_available() -> bool: reports whether an audio device is open.
- audio2d_error() -> string: returns the runtime's latest audio error.
- audio2d_tone(frequency, duration_ms, volume): plays a generated tone; invalid/non-positive frequency or duration is ignored.

## Supported files

The built-in fallback decoder accepts RIFF/WAVE files with PCM 8-bit or 16-bit
samples, mono or stereo, at any sample rate. Compressed formats such as MP3 and
Ogg require the optional SDL2_mixer runtime library; without it, keep WAV
assets in the repository.

Loaded samples are converted to the format the active audio device opened with
and resampled when the device rate differs from the file rate.

## Diagnostics

Audio availability depends on the host libraries and a working output device.
A successfully compiled program may still run silently on a host without audio
support; check audio2d_available(), the handle returned from audio2d_load(),
and audio2d_error() where relevant. Always unload loaded samples when ownership
ends.
