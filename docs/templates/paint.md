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

The source implements pencil/brush, eraser, straight line, rectangle and fill tools; an on-screen color palette; adjustable brush size; undo/redo checkpoints; BMP save and load; fullscreen toggle; and a drawing canvas of 1024×720 pixels. The exact UI controls are defined by the checked-in `examples/nexus_paint.nx`; do not rely on keyboard shortcut lists copied from older drafts.

The app writes `nexus_paint.bmp` relative to its working directory. Choose a writable working folder if you plan to save drawings.

## Packaging

The repository CI also builds a Windows x64 NEXUS Paint executable and portable ZIP. The ZIP is the recommended standalone download because it keeps the runtime DLLs and editable `.nx` source beside the executable. See the [Downloads page](../downloads.md) when GitHub Pages is enabled.

The canvas currently saves BMP. PNG export, layers, selections, vector paths and arbitrary image formats are not included in the current source.
