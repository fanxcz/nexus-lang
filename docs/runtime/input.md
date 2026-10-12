# Input

Keyboard, mouse, and gamepad handling.

## Keyboard

```nx
import runtime.input

// Key states
input_key_down(KEY_SPACE)      // Held down
input_key_pressed(KEY_SPACE)   // Just pressed this frame
input_key_released(KEY_SPACE)  // Just released this frame

// Key codes
KEY_A, KEY_B, ..., KEY_Z
KEY_0, KEY_1, ..., KEY_9
KEY_F1, ..., KEY_F12
KEY_SPACE, KEY_ENTER, KEY_ESCAPE
KEY_TAB, KEY_SHIFT, KEY_CTRL, KEY_ALT
KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
KEY_W, KEY_A, KEY_S, KEY_D  // Common aliases

// Get key name
let name = input_key_name(KEY_SPACE)  // "Space"
```

## Mouse

```nx
// Position (screen coordinates)
let x = input_mouse_x()
let y = input_mouse_y()

// Position (virtual UI coordinates - if gfx_set_ui_resolution used)
let ui_x = input_mouse_ui_x()
let ui_y = input_mouse_ui_y()

// Buttons
input_mouse_down(MOUSE_LEFT)     // Held
input_mouse_pressed(MOUSE_LEFT)  // Clicked
input_mouse_released(MOUSE_LEFT) // Released

// Buttons: MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_X1, MOUSE_X2

// Scroll wheel
let scroll_x = input_mouse_scroll_x()
let scroll_y = input_mouse_scroll_y()

// Cursor visibility
input_set_cursor_visible(true)
input_set_cursor_grabbed(true)  // Lock to window

// Set cursor position
input_warp_mouse(400, 300)
```

## Gamepad

```nx
// Check connected
if input_gamepad_connected(0) {  // Index 0-3
    // Buttons
    input_gamepad_down(0, GPAD_A)
    input_gamepad_pressed(0, GPAD_START)
    
    // Buttons: GPAD_A, GPAD_B, GPAD_X, GPAD_Y
    // GPAD_LB, GPAD_RB, GPAD_LT, GPAD_RT
    // GPAD_START, GPAD_BACK, GPAD_GUIDE
    // GPAD_LEFT_STICK, GPAD_RIGHT_STICK
    // GPAD_DPAD_UP, GPAD_DPAD_DOWN, GPAD_DPAD_LEFT, GPAD_DPAD_RIGHT
    
    // Axes (-1.0 to 1.0)
    let lx = input_gamepad_axis(0, GPAD_AXIS_LEFT_X)
    let ly = input_gamepad_axis(0, GPAD_AXIS_LEFT_Y)
    let rx = input_gamepad_axis(0, GPAD_AXIS_RIGHT_X)
    let ry = input_gamepad_axis(0, GPAD_AXIS_RIGHT_Y)
    let lt = input_gamepad_axis(0, GPAD_AXIS_TRIGGER_LEFT)
    let rt = input_gamepad_axis(0, GPAD_AXIS_TRIGGER_RIGHT)
    
    // Vibration
    input_gamepad_rumble(0, 0.5, 0.5, 500)  // (weak, strong, duration_ms)
}

// Get name
let name = input_gamepad_name(0)  // "Xbox Controller"
```

## Text Input

```nx
// Enable text input (shows on-screen keyboard on mobile)
input_text_input_start()

// Get typed text this frame
let text = input_text_input_get()

// Disable
input_text_input_stop()
```

## Event Polling

```nx
while !gfx_should_close() {
    input_poll_events()  // Must call once per frame
    
    // Now input_* functions work
    if input_key_pressed(KEY_ESCAPE) { break }
    // ...
}
```

## Complete Example

```nx
import runtime.gfx
import runtime.input

fn main() {
    gfx_init(800, 600, "Input Demo")
    defer gfx_quit()

    while !gfx_should_close() {
        input_poll_events()
        
        let dt = gfx_delta_time()
        
        // Keyboard movement
        let mut x = 400.0
        let mut y = 300.0
        let speed = 300.0
        
        if input_key_down(KEY_W) { y -= speed * dt }
        if input_key_down(KEY_S) { y += speed * dt }
        if input_key_down(KEY_A) { x -= speed * dt }
        if input_key_down(KEY_D) { x += speed * dt }
        
        // Mouse click
        if input_mouse_pressed(MOUSE_LEFT) {
            println("Click at: {}, {}", input_mouse_x(), input_mouse_y())
        }
        
        // Gamepad
        if input_gamepad_connected(0) {
            if input_gamepad_pressed(0, GPAD_A) {
                println("A button pressed!")
            }
            let lx = input_gamepad_axis(0, GPAD_AXIS_LEFT_X)
            x += lx * speed * dt
        }
        
        gfx_begin_frame()
        gfx_clear(0x1a1a2e)
        gfx_draw_rect(x - 16, y - 16, 32, 32, 0xffffff)
        gfx_draw_text(10, 10, "WASD / Gamepad to move, Click mouse", 0x888888)
        gfx_end_frame()
    }
}
```