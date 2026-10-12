# Audio

NEXUS audio uses **SDL2_mixer** for sound effects and music playback.

## Initialization

```nx
import runtime.audio

fn main() {
    audio_init()
    defer audio_quit()
    
    // ...
}
```

## Sound Effects

```nx
// Load WAV/OGG sound effect
let jump_sfx = audio_load_sound("assets/jump.wav")
let coin_sfx = audio_load_sound("assets/coin.ogg")

// Play once
audio_play_sound(jump_sfx)

// Play on specific channel (-1 = first free)
audio_play_sound_on_channel(coin_sfx, 1)

// Volume (0.0 - 1.0)
audio_set_sound_volume(jump_sfx, 0.5)

// Free
audio_free_sound(jump_sfx)
```

## Music

```nx
// Load music (OGG, MP3, MOD, etc.)
let music = audio_load_music("assets/theme.ogg")

// Play (loops = -1 for infinite)
audio_play_music(music, -1)

// Pause/Resume
audio_pause_music()
audio_resume_music()

// Stop
audio_stop_music()

// Volume
audio_set_music_volume(0.7)

// Free
audio_free_music(music)
```

## Master Volume

```nx
// Global volume (affects all sounds/music)
audio_set_master_volume(0.8)
```

## Tone Generation (Procedural)

```nx
// Generate simple tone: frequency(Hz), duration(ms), volume(0-1)
audio_play_tone(440.0, 200, 0.5)  // A4 note

// Useful for retro-style sound effects
audio_play_tone(800.0, 100, 0.3)  // Jump
audio_play_tone(200.0, 300, 0.4)  // Hit
audio_play_tone(1200.0, 50, 0.2)  // Coin
```

## Channel Management

```nx
// Reserve channels for important sounds
audio_reserve_channels(4)  // Channels 0-3 reserved

// Get channel status
let playing = audio_channel_playing(0)

// Stop specific channel
audio_halt_channel(0)

// Stop all
audio_halt_all()
```

## Supported Formats

| Format | Sound Effects | Music |
|--------|---------------|-------|
| WAV    | ✅            | ✅    |
| OGG    | ✅            | ✅    |
| MP3    | ❌            | ✅    |
| MOD/XM | ❌            | ✅    |
| FLAC   | ✅            | ✅    |

## Complete Example

```nx
import runtime.audio
import runtime.gfx
import runtime.input

fn main() {
    gfx_init(400, 300, "Audio Demo")
    defer gfx_quit()
    
    audio_init()
    defer audio_quit()

    let jump = audio_load_sound("assets/jump.wav")
    let coin = audio_load_sound("assets/coin.ogg")
    let music = audio_load_music("assets/bgm.ogg")
    
    audio_play_music(music, -1)
    audio_set_music_volume(0.5)

    while !gfx_should_close() {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        gfx_draw_text(10, 10, "SPACE = Jump  |  C = Coin  |  M = Toggle Music", 0xffffff)
        gfx_end_frame()
        
        input_poll_events()
        
        if input_key_pressed(KEY_SPACE) {
            audio_play_sound(jump)
        }
        if input_key_pressed(KEY_C) {
            audio_play_sound(coin)
        }
        if input_key_pressed(KEY_M) {
            if audio_music_playing() {
                audio_pause_music()
            } else {
                audio_resume_music()
            }
        }
    }
}
```

## Best Practices

1. **Load once, play many** - Load sounds at startup, not in game loop
2. **Use channels wisely** - Reserve channels for UI/player sounds
3. **Free resources** - Call `audio_free_sound/music` before exit (or rely on `audio_quit()`)
4. **Volume hierarchy** - Master → Music/Sound → Individual
5. **Procedural tones** - Great for retro games, zero asset size

## Troubleshooting

| Issue | Fix |
|-------|-----|
| No sound | Check `audio_init()` called, volume > 0 |
| Music not looping | Use `audio_play_music(music, -1)` |
| Crackling | Reduce polyphony, check sample rates |
| MP3 not playing | Use OGG for music (better support) |