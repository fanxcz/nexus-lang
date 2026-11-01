
# Scene Snapshots

The `scene_*` helpers store and load snapshots of built-in ECS entity data. They are not a full scene lifecycle/transition manager: there are no enter/exit callbacks, registered named scenes or transition animations.

## API

- `scene_create()` creates a snapshot ID.
- `scene_add(scene, entity)` copies the entity's current built-in fields into the snapshot.
- `scene_save(scene, path)` writes the snapshot to a text file and returns a success value.
- `scene_load(path)` reads a snapshot and returns a scene ID, or `0` if loading fails.
- `scene_apply(scene)` replaces the live ECS entity registry with the snapshot and returns the number of recreated entities. It returns `-1` for an invalid scene handle.
- `scene_clear(scene)` clears snapshot data.
- `scene_destroy(scene)` releases the snapshot.

A snapshot includes supported built-in fields such as position, velocity, size, colour and texture ID. Saving preserves float round-trip precision and returns `false` when the output cannot be written or the snapshot contains non-finite numeric data. Loading accepts at most 100,000 entities and rejects truncated records, non-finite values and trailing non-whitespace data. A failed load returns `0` without creating a partial snapshot.\n\nApplying a snapshot builds the replacement ECS registry before swapping it into the runtime. An invalid handle or allocation failure returns `-1` without clearing the current live world. A successful apply invalidates all existing ECS entity handles because the live entity registry is replaced. Snapshot texture IDs are copied as numeric references; texture resources are not serialized or recreated, so referenced textures must already exist in the active runtime.

This is a deliberately small serialization/apply API, not a general-purpose editor scene graph, resource manager or save-game system.
