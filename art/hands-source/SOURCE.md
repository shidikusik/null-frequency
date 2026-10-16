# Editable first-person arms sources

This directory contains authoring files and conversion experiments. It is kept
in the source repository, outside `assets`, and is not copied into game packages.
Blender is needed only when editing or regenerating an asset.

## Runtime arms: para

- Creator: **para**; original human mesh and texture: **MakeHuman team**.
- Primary upload: <https://opengameart.org/content/fps-arms-rigged-only>.
- Original download: <https://opengameart.org/sites/default/files/fps%20arms.7z>.
- License: **CC0 1.0 Universal**, explicitly listed on the creator's upload.
- License checked on 2026-10-16; full legal text: [CC0-1.0.txt](CC0-1.0.txt).

`para/` preserves the original archive, two Blender scenes, two FBX exports
and `new_diff.png`. The upload credits MakeHuman for the original geometry
and texture. The rig and UV/texture cleanup are by para. `para/convert.py`
is the project's Blender conversion script; `para/para.glb` and
`para/raylib-bind-pose.json` are conversion/rig inspection intermediates.
The final playable file is [`assets/models/hands/hands.glb`](../../assets/models/hands/hands.glb).

Regenerate the intermediate GLB from the supplied rest scene:

```sh
blender --background --disable-autoexec --python art/hands-source/para/convert.py
```

The script reopens the saved rest scene, removes authoring constraints/control
bones, normalizes to metres, embeds the diffuse texture and exports a glTF 2.0
GLB beside the script. The final runtime adaptation and hand animation are
documented in [`assets/models/hands/SOURCE.md`](../../assets/models/hands/SOURCE.md).
Reload, walking, recoil and inspect motion are implemented by this game;
the source pack's sample animation is not a production reload animation.

## Unused alternative: DevMops

- Creator: **DevMops**.
- Primary upload: <https://opengameart.org/content/low-poly-arms-rigged>.
- License: **CC0 1.0 Universal**, explicitly listed on the creator's upload.
- License checked on 2026-10-16; full legal text: [CC0-1.0.txt](CC0-1.0.txt).

`devmops/` preserves the original ZIP, editable `.blend`, FBX and
`Hand Texture.png`. This is an earlier rig candidate and is not used by the
game. The creator describes the rig as made with Blender's Rigify add-on.

The CC0 assets retain their CC0 dedication; this project's original conversion
script and animation code use the repository's [MIT license](../../LICENSE).

Original archive SHA-256 checksums:

```text
31f6c7bd5caea8856c4aafca8461f38a3c8bfdd3d8f05c898e403b9475e54562  para/fps-arms.7z
a685cd52d98f884565ae2d0b29d93cecebeff614c4ee7f57055502267a86887a  devmops/arms-low-poly-rigged.zip
```
