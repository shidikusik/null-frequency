# Animated entity

`entity.glb` contains the detailed, faceless humanoid mannequin and skeletal
animation from **Universal Animation Library (Standard)** by **Quaternius**.
The original geometry is preserved. The orange/purple material palette was
changed to neutral mineral grey and dark green-grey for NULL FREQUENCY, and
unused clips and buffer data were removed. No Blender, runtime retargeting, or
external textures are required.

## Source and permission

- Creator's official pack and CC0 declaration:
  <https://quaternius.com/packs/universalanimationlibrary.html>
- Creator's own OpenGameArt upload, including the original Standard archive:
  <https://opengameart.org/content/universal-animation-library>
- Original archive:
  <https://opengameart.org/sites/default/files/universal_animation_librarystandard.zip>
- Downloaded glTF mirror, preserving the creator's CC0 license, pinned to commit
  `e24c23cf2a1323488a3faa226ea7ea21f644b73e`:
  <https://github.com/J-Ponzo/gltf-universal-animation-library/tree/e24c23cf2a1323488a3faa226ea7ea21f644b73e/glTF>

The source files are `AnimationLibrary_Godot_Standard.gltf` and
`AnimationLibrary_Godot_Standard.bin`. This is the freely distributed Standard
pack, not the paid Pro or Source pack. Both the official publisher page and the
creator's OpenGameArt upload identify it as **CC0 1.0 Universal**. Redistribution,
modification, and commercial use are permitted. The full dedication is included
in [CC0-1.0.txt](CC0-1.0.txt).

Verified and downloaded on 2026-10-15. Attribution is provided as a courtesy:
**Humanoid model and animations by Quaternius (CC0).**

## Integration

- glTF 2.0 binary, 941,624 bytes; embedded vertex and animation data.
- 13,743 triangles, 53 skeletal bones, 2 mesh primitives/materials.
- Y is up; **+Z is forward**. Origin is at the feet; no root motion is applied by
  the game. The character's position must be controlled by the gameplay core.
- Original bind-pose bounds in metres:
  `(-0.972227, 0.000461, -0.164154)` to `(0.972227, 1.829178, 0.205444)`.
- Original height is approximately **1.829 m**. Uniform scale `1.072` gives a
  1.96 m entity. The wide bind-pose bounds are the outstretched T-pose arms;
  the walk and idle clips lower the arms into their natural positions.
- Each independent enemy needs its own `Model` instance when using raylib's
  CPU `UpdateModelAnimation`; the animation definitions may be shared.

| Index | Clip | Source duration | Frames loaded by raylib 5.5 |
| --- | --- | --- | --- |
| 0 | `Idle_Loop` | 2.5 s | 148 |
| 1 | `Walk_Loop` | 1.3333 s | 79 |
| 2 | `Punch_Cross` | 1.0 s | 59 |
| 3 | `Punch_Jab` | 0.8333 s | 50 |
| 4 | `Hit_Chest` | 0.3333 s | 20 |
| 5 | `Death01` | 2.375 s | 140 |

Loop idle/walk; play attack, hit, and death clips once. Clip counts here were
measured with the game's pinned raylib 5.5 build, rather than inferred from
the source sample count. Sampling at approximately 60 frames per second is
appropriate.

## Rebuilding and validation

Download the two source files above into the same directory, then run:

```sh
python assets/models/entity/prepare_entity.py \
  /path/to/AnimationLibrary_Godot_Standard.gltf \
  assets/models/entity/entity.glb
```

`prepare_entity.py` uses only the Python standard library. It selects the six
clips listed above, preserves their skeleton and mesh bindings, removes unused
accessors and buffer views, adjusts the two material colours, and embeds
everything into one GLB.

Validated with raylib 5.5 `LoadModel`, `LoadModelAnimations`, and
`IsModelAnimationValid` for all six clips. All clips change the actual animated
vertex positions. A rendered walk frame was visually inspected; the body,
limbs, hands, feet, and floor alignment are intact. The walk midpoint bounds
are `(-0.266153, 0.000397, -0.416696)` to `(0.364100, 1.778290, 0.523663)`.

SHA-256 of the packed asset:
`cd5cf5636dba8d5d453645ac8724a087721f720a1859a8769c4258478ee8b588`.
