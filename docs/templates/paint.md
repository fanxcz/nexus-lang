# Paint Template

Nexus Paint - raster graphics editor.

## Create

```bash
nexus new MyPaint --template paint
cd MyPaint
nexus build --release
nexus run
```

## Features

- **Canvas**: 1024x720 pixels
- **Tools**: Pencil, Eraser, Line, Rectangle, Fill
- **Colors**: 16-color palette + custom picker
- **Brush**: Size 1-50 pixels
- **History**: Unlimited undo/redo
- **File**: BMP save/load
- **Shortcuts**: 
  - `B` - Brush
  - `E` - Eraser
  - `L` - Line
  - `R` - Rectangle
  - `F` - Fill
  - `Z` - Undo
  - `Y` - Redo
  - `S` - Save
  - `O` - Load

## Structure

```
MyPaint/
├── nexus.toml
├── src/
│   ├── main.nx       # App entry
│   ├── canvas.nx     # Canvas logic
│   ├── tools.nx      # Tool implementations
│   ├── palette.nx    # Color palette
│   └── history.nx    # Undo/redo
├── assets/
│   └── icons/
└── tests/
```

## Extending

```nx
// Add new tool
enum Tool {
    Pencil,
    Eraser,
    Line,
    Rectangle,
    Fill,
    // Add custom
    SprayCan,
    Gradient,
}

// Implement in tools.nx
fn tool_spray_can(canvas: *Canvas, x: i32, y: i32) {
    // Spray paint logic
}
```

## Export Formats

Currently supports BMP. To add PNG:

```nx
// Add stb_image_write integration
extern "C" {
    fn stbi_write_png(filename: *const i8, w: i32, h: i32, comp: i32, data: *const u8, stride: i32) -> i32
}

fn save_png(canvas: *Canvas, path: str) {
    unsafe {
        stbi_write_png(path.as_c_str(), canvas.width, canvas.height, 4, canvas.pixels, canvas.width * 4)
    }
}
```