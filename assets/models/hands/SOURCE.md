# First-person hands and arms

- **Asset:** fps arms (rigged only)
- **Author:** [para](https://opengameart.org/users/para), with the original human mesh and texture credited by the author to the **MakeHuman team**.
- **Primary upload:** <https://opengameart.org/content/fps-arms-rigged-only>
- **Original archive:** <https://opengameart.org/sites/default/files/fps%20arms.7z>
- **License:** **CC0 1.0 Universal**, explicitly listed on the primary upload; verified 2026-10-16. The full dedication is preserved in [CC0-1.0.txt](CC0-1.0.txt).
- **Credit:** FPS arms by para / MakeHuman team, CC0; Blender conversion and first-person animation adaptation for NULL FREQUENCY.

`hands.glb` is a self-contained glTF 2.0 binary with an embedded 1024 × 1024 skin texture. It contains the two textured arms and a 49-joint deform rig, including separately articulated finger joints. The original editable Blender scenes, FBX files, archive, texture and conversion script are preserved in [`art/hands-source/para`](../../../art/hands-source/para); they are excluded from binary game packages.

The supplied rest scene was converted with Blender: authoring constraints and control bones were removed, the hands were connected to their forearms, the armature and geometry were normalized together to metres, and the diffuse texture was embedded. The resulting GLB has no baked animation clips. Its `hand.L` / `hand.R`, `upper_arm.*`, `forearm.*`, `f_index.*`, `f_middle.*`, `f_ring.*`, `f_pinky.*` and `thumb.*` joints are posed at runtime by [`src/weapon_view.hpp`](../../../src/weapon_view.hpp).

The game supplies two-arm inverse kinematics, a two-handed revolver grip with individual finger curls, aim-down-sights transitions, walking and sprinting motion, mouse inertia, recoil, weapon inspection and a staged cylinder reload. These are the game's animations, rather than animations supplied by the original asset author. Reload cartridges and the revolver mechanism are separate from this arms model.

Regenerate the converted GLB from the repository root:

```sh
blender --background --disable-autoexec --python art/hands-source/para/convert.py
cp art/hands-source/para/para.glb assets/models/hands/hands.glb
```

Runtime GLB SHA-256:

```text
2f1d9284b3d561752bb69e32783576715817978cd3ff08166bfaa63077a2acbc
```
