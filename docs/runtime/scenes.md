# Scene Snapshots

The `scene_*` helpers store and load snapshots of built-in ECS entity data. They are not a full scene lifecycle/transition manager: there are no enter/exit callbacks, registered named scenes, transition animations or automatic entity re-instantiation API.

## API

- `scene_create()` creates a snapshot ID.
- `scene_add(scene, entity)` copies the entity's current built-in fields into the snapshot.
- `scene_save(scene, path)` writes the snapshot to a text file and returns a success value.
- `scene_load(path)` reads a snapshot and returns a scene ID, or 0 if loading fails.
- `scene_clear(scene)` clears snapshot data.
- `scene_destroy(scene)` releases the snapshot.

A snapshot includes supported built-in fields such as position, velocity, size, colour and texture ID. Loading a snapshot does not automatically recreate or replace live ECS entities, and the public API does not expose an iterator over loaded snapshot entries. Treat it as experimental serialization support rather than a complete save-game manager.
