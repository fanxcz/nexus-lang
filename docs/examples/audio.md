# Audio Examples

Audio playback and synthesis demos.

## Audio Demo

`examples/audio_demo.nx` — Basic audio playback.

**Features:**
- Sound effects (WAV/OGG)
- Background music
- Volume controls
- Pitch variation
- Channel management

**Run:**
```bash
nexus run --example audio_demo
```

## Code Example

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
        gfx_draw_text(10, 10, "SPACE=Jump  C=Coin  M=Music", 0xffffff)
        gfx_end_frame()
        
        input_poll_events()
        
        if input_key_pressed(KEY_SPACE) { audio_play_sound(jump) }
        if input_key_pressed(KEY_C) { audio_play_sound(coin) }
        if input_key_pressed(KEY_M) {
            if audio_music_playing() { audio_pause_music() }
            else { audio_resume_music() }
        }
    }
}
```

## Tone Generation

```nx
// Procedural sounds (retro-style)
audio_play_tone(440.0, 100, 0.5)  // A4 note
audio_play_tone(880.0, 50, 0.3)   // A5
audio_play_tone(220.0, 200, 0.4)  // A3
```

## Supported Formats

| Format | Sound Effects | Music |
|--------|---------------|-------|
| WAV    | ✅ | ✅ |
| OGG    | ✅ | ✅ |
| MP3    | ❌ | ✅ |
| MOD/XM | ❌ | ✅ |
| FLAC   | ✅ | ✅ |

## Best Practices

1. **Load once** at startup, not in game loop
2. **Use channels** for simultaneous sounds
3. **Pool sounds** for frequent effects (bullets, footsteps)
4. **Set volumes** per-category (SFX, music, UI)
5. **Clean up** with `audio_free_sound()` or `audio_quit()`