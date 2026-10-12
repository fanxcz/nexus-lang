# UI (Immediate Mode)

Immediate-mode GUI for debug tools, editors, and simple interfaces.

## Basic Usage

```nx
import runtime.ui

fn main() {
    gfx_init(800, 600, "UI Demo")
    defer gfx_quit()
    
    let mut name = "Player"
    let mut health = 100
    let mut volume = 0.5
    let mut show_demo = false
    
    while !gfx_should_close() {
        input_poll_events()
        let dt = gfx_delta_time()
        
        ui_begin()
        
        // Window
        ui_window("Game Settings", 10, 10, 300, 400, {
            ui_label("Player Name:")
            ui_input_text("##name", &mut name, 32)
            
            ui_separator()
            
            ui_label("Health: {health}")
            ui_slider_int("##health", &mut health, 0, 100)
            
            ui_separator()
            
            ui_label("Volume: {volume:.0%}")
            ui_slider_float("##vol", &mut volume, 0.0, 1.0)
            
            ui_separator()
            
            if ui_button("Show Demo") { show_demo = true }
            if ui_button("Reset") { health = 100; volume = 0.5 }
            
            ui_separator()
            
            ui_checkbox("Fullscreen", &mut fullscreen)
        })
        
        // Demo window
        if show_demo {
            ui_window("Demo", 350, 10, 400, 300, {
                ui_label("This is a demo window")
                if ui_button("Close") { show_demo = false }
            })
        }
        
        ui_end()
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        ui_render()
        gfx_end_frame()
    }
}
```

## Widgets

```nx
// Text
ui_label("Hello World")
ui_text("Multiline\ntext\nsupport")

// Input
ui_input_text("##id", &mut string, max_len)
ui_input_int("##id", &mut int)
ui_input_float("##id", &mut float)

// Buttons
if ui_button("Click Me") { ... }
if ui_small_button("X") { ... }
if ui_arrow_button("##left", DIR_LEFT) { ... }

// Checkbox
ui_checkbox("Enable", &mut bool)

// Radio
ui_radio_button("Option A", &mut selected, 0)
ui_radio_button("Option B", &mut selected, 1)

// Sliders
ui_slider_int("##id", &mut val, min, max)
ui_slider_float("##id", &mut val, min, max)
ui_drag_int("##id", &mut val, speed, min, max)
ui_drag_float("##id", &mut val, speed, min, max)

// Combo
ui_combo("##combo", &mut selected, ["A", "B", "C"])

// List box
ui_list_box("##list", &mut selected, ["Item 1", "Item 2", "Item 3"])

// Color picker
ui_color_edit3("##color", &mut color)
ui_color_edit4("##color", &mut color)  // With alpha

// Progress
ui_progress_bar(0.75, 200, 20)

// Tree
ui_tree_node("Folder", {
    ui_label("File 1")
    ui_label("File 2")
})

// Tabs
ui_tab_bar("##tabs", {
    ui_tab_item("Tab 1", { ui_label("Content 1") })
    ui_tab_item("Tab 2", { ui_label("Content 2") })
})
```

## Layout

```nx
// Columns
ui_columns(2, {
    ui_label("Left")
    ui_next_column()
    ui_label("Right")
})

// Indent
ui_indent(20)
ui_label("Indented")
ui_unindent(20)

// Spacing
ui_spacing()
ui_separator()
ui_dummy(0, 20)  // Empty space

// Groups
ui_group("Group Name", {
    ui_label("Inside group")
})

// Child window (scrollable)
ui_child("##scroll", 0, 200, true, {
    for i in 0..100 {
        ui_label("Line {i}")
    }
})
```

## Styling

```nx
// Push style
ui_push_style_color(ui.COLOR_BUTTON, 0xff4488aa)
ui_push_style_var(ui.STYLE_FRAME_ROUNDING, 5.0)

ui_button("Styled")

ui_pop_style_color()
ui_pop_style_var()

// Global style
ui_style_colors_dark()   // Default
ui_style_colors_light()  // Light theme
ui_style_colors_classic() // Classic
```

## Input Handling

```nx
// Keyboard navigation
ui_nav_init()

// Focus
ui_set_keyboard_focus_here()

// Item state
let hovered = ui_is_item_hovered()
let active = ui_is_item_active()
let focused = ui_is_item_focused()
let clicked = ui_is_item_clicked()
let edited = ui_is_item_edited()
let visible = ui_is_item_visible()

// Rect
let rect = ui_get_item_rect_min(), ui_get_item_rect_max()
```

## Fonts

```nx
// Load font
let font = ui_font_load("assets/font.ttf", 18)

// Set default
ui_font_set_default(font)

// Multiple fonts
ui_font_push(font_small)
ui_label("Small text")
ui_font_pop()

ui_font_push(font_large)
ui_label("Large text")
ui_font_pop()
```

## Example: Debug Console

```nx
struct DebugConsole {
    lines: []str,
    input: str,
    visible: bool,
}

fn debug_console_update(console: &mut DebugConsole) {
    if input_key_pressed(KEY_F1) {
        console.visible = !console.visible
    }
    if !console.visible { return }
    
    ui_window("Console", 10, 10, 780, 400, {
        ui_child("##log", 0, 300, true, {
            for line in console.lines {
                ui_text(line)
            }
        })
        ui_separator()
        
        if ui_input_text("##cmd", &mut console.input, 256, 
            INPUT_TEXT_ENTER_RETURNS_TRUE) 
        {
            console.lines.push("> " + console.input)
            execute_command(console.input)
            console.input = ""
        }
    })
}
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Tab` | Next widget |
| `Shift+Tab` | Previous widget |
| `Enter` | Activate button/confirm |
| `Esc` | Cancel/close popup |
| `Arrow keys` | Navigate/slider adjust |
| `Home/End` | Start/end of input |
| `Ctrl+C/V/X` | Copy/paste/cut |

## Best Practices

1. **Call `ui_begin()` once per frame** before any widgets
2. **Call `ui_end()` once per frame** after all widgets
3. **Call `ui_render()`** in render pass (after `gfx_begin_frame()`)
4. **Unique IDs** - use `##id` suffix for widgets with same label
4. **Don't store UI state** - immediate mode recomputes each frame
5. **Keep frames fast** - UI runs every frame