# Scenes

Scene management for game states (menu, gameplay, pause, etc.).

## Scene Trait

```nx
trait Scene {
    fn on_enter(&mut self, world: *World)
    fn on_exit(&mut self, world: *World)
    fn update(&mut self, world: *World, dt: f32)
    fn render(&mut self, world: *World)
}
```

## Scene Manager

```nx
import runtime.scenes

fn main() {
    let manager = scenes_create()
    defer scenes_destroy(manager)
    
    // Register scenes
    scenes_register(manager, "menu", MenuScene::new())
    scenes_register(manager, "game", GameScene::new())
    scenes_register(manager, "pause", PauseScene::new())
    scenes_register(manager, "gameover", GameOverScene::new())
    
    // Start with menu
    scenes_switch(manager, "menu")
    
    while !gfx_should_close() {
        let dt = gfx_delta_time()
        
        scenes_update(manager, dt)
        scenes_render(manager)
        
        gfx_end_frame()
    }
}
```

## Scene Implementation

```nx
struct MenuScene {
    selected: i32,
}

impl Scene for MenuScene {
    fn new() -> Self {
        Self { selected: 0 }
    }
    
    fn on_enter(&mut self, world: *World) {
        println("Entering Menu")
        audio_play_music(menu_music, -1)
    }
    
    fn on_exit(&mut self, world: *World) {
        audio_stop_music()
    }
    
    fn update(&mut self, world: *World, dt: f32) {
        input_poll_events()
        
        if input_key_pressed(KEY_UP) {
            self.selected = (self.selected - 1).max(0)
        }
        if input_key_pressed(KEY_DOWN) {
            self.selected = (self.selected + 1).min(2)
        }
        if input_key_pressed(KEY_ENTER) {
            match self.selected {
                0 => scenes_switch(world, "game"),
                1 => scenes_switch(world, "options"),
                2 => gfx_quit(),
            }
        }
    }
    
    fn render(&mut self, world: *World) {
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        
        let items = ["Start Game", "Options", "Quit"]
        for (i, item) in items.enumerate() {
            let color = if i == self.selected { 0xffff00 } else { 0xffffff }
            gfx_draw_text(300, 200 + i * 50, item, color)
        }
        
        gfx_end_frame()
    }
}
```

## Transitions

```nx
// Fade transition
scenes_switch_with_transition(manager, "game", TRANSITION_FADE, 1.0)

// Slide transition
scenes_switch_with_transition(manager, "game", TRANSITION_SLIDE_LEFT, 0.5)

// Custom transition
scenes_switch_with_transition(manager, "game", TRANSITION_CUSTOM, 1.0,
    |progress| { /* custom render */ })
```

## Stack-Based (Pause Menu)

```nx
// Push pause on top of game
scenes_push(manager, "pause")

// Pop to return
scenes_pop(manager)

// Replace top
scenes_replace(manager, "gameover")
```

## Data Passing

```nx
// Pass data to next scene
scenes_switch_with_data(manager, "gameover", { score: 1500 })

// In target scene
fn on_enter(&mut self, world: *World) {
    let data = scenes_get_enter_data(world)
    self.final_score = data.get_i32("score")?
}
```

## Example Structure

```
scenes/
├── menu.nx
├── gameplay.nx
├── pause.nx
├── gameover.nx
├── settings.nx
└── loading.nx
```

## Best Practices

1. **One scene per file** - clean separation
2. **Minimal `on_enter`/`on_exit`** - only setup/teardown
3. **Use resources for shared data** - ECS resources, not globals
4. **Async loading** - LoadingScene shows progress while assets load
5. **Pause vs Switch** - Use `push/pop` for overlays (pause, dialogs)