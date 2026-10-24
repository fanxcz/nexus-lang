# Experimental Scene Editor Source

The file `examples/templates/editor.nx` is a small experimental **2D scene editor**. It draws a simple viewport/inspector layout, creates a couple of colored ECS entities, and includes SAVE, LOAD and ADD BOX buttons based on the current scene-snapshot and UI helper APIs.

**Important:** the current `nexus new --template` CLI does not accept `editor` as a template name. To try this source, create a supported app project and copy the example into its entry point:

```bash
nexus new MyEditor --template app
cd MyEditor
copy ..\\examples\\templates\\editor.nx src\\main.nx
nexus check
nexus build
nexus run
```

On Linux/macOS, use `cp ../examples/templates/editor.nx src/main.nx` instead of `copy` (adjust the source path to the repository checkout).

## Current scope and limitations

The source demonstrates absolute-coordinate buttons, 2D drawing, ECS entity helpers and experimental `scene_save` / `scene_load` functions. Those scene functions serialize a snapshot of built-in entity fields; they are not a complete scene graph or automatic reload of entity instances.

This is not a source-code IDE and does not implement syntax highlighting, multiple source tabs, find/replace, LSP, an asset browser or a general inspector framework. For code editing, use [NEXUS Studio](https://github.com/fanxcz/nexus-lang/tree/main/.github/project-tools/nexus-studio), the separate desktop IDE project.
