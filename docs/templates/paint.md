# Paint Template

The `paint` template copies the repository's NEXUS Paint raster editor into `src/main.nx`. It uses the CPU-side canvas API, a virtual 1600×900 UI space and a mouse-driven tool panel.

## Create and run

```bash
nexus new MyPaint --template paint
cd MyPaint
nexus check
nexus build --release
nexus run
```

## Current features

The source implements pencil/brush, eraser, straight line, rectangle and fill tools; an on-screen color palette; adjustable brush size; fullscreen toggle; and a 1024×720 CPU-side drawing canvas. A base canvas can be combined with up to three transparent raster layers. Each layer has its own undo/redo history, can be selected independently, and can be hidden or shown; file export flattens the visible layers in source-over order.

The import path field accepts image paths relative to the application's working directory. BMP import is available through SDL2; other formats such as PNG/JPEG require the optional SDL2_image runtime. BMP export is implemented by NEXUS. PNG export is enabled only when the installed SDL2_image library provides `IMG_SavePNG` and the SDL surface-format API; otherwise Paint reports the missing capability rather than producing a fake PNG.

The canvas runtime caps an image import at 16 megapixels and maintains at most 32 MiB of undo snapshots per canvas; available history depth therefore depends on canvas size. The app writes `nexus_paint.bmp` and, when PNG support is available, `nexus_paint.png` relative to its working directory. Choose a writable working folder if you plan to save drawings. The exact UI controls are defined by the checked-in `examples/nexus_paint.nx`.

## Packaging

The repository CI also builds a Windows x64 NEXUS Paint executable and portable ZIP. The ZIP is the recommended standalone download because it keeps the runtime DLLs and editable `.nx` source beside the executable. See the [Downloads page](../downloads.md) when GitHub Pages is enabled.

Selections, vector paths, and nondestructive adjustment layers are not included. Layers are raster canvases with per-layer history; exported images are flattened.
