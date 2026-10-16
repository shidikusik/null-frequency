# Revolver model

- **Asset:** Revolver (internal mesh name `Revolver_3`)
- **Author:** [Quaternius](https://poly.pizza/u/Quaternius)
- **Primary asset page:** https://poly.pizza/m/9C26wSpMS0
- **Original pack page:** https://quaternius.com/packs/ultimategun.html
- **Download:** https://static.poly.pizza/9e728565-67a3-44db-9567-982320abff09.glb
- **Downloaded:** 2026-10-15
- **License:** [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/). The specific model page explicitly lists Public Domain (CC0), with an upload date of 2021-08-26. This records the license of this particular asset, rather than inferring it from the creator's current general license.
- **License text:** `CC0-1.0.txt` in this directory.
- **Credit:** Revolver by Quaternius, CC0 1.0; runtime orientation and scale adjusted for NULL FREQUENCY.

`weapon.glb` is self-contained and has no external texture dependencies. The mesh has 2,771 vertices and 1,434 triangles, divided among four material primitives. Raylib 5.5 imports these as four meshes and five materials (including its added default material). There are no skeletons or embedded animations; recoil/reload motion is applied by the game.

Only the root transform was adjusted: the muzzle points along **-Z**, the grip points toward **-Y**, and overall front-to-back length is **0.42 m**. The origin remains near the grip/cylinder so the game can pivot the model for recoil and reloading.

Raylib 5.5 `GetModelBoundingBox` after normalization:

| Axis | Minimum | Maximum |
| --- | ---: | ---: |
| X | -0.031145 | 0.031138 |
| Y | -0.070907 | 0.136977 |
| Z | -0.374909 | 0.045091 |

Approximate muzzle center: `(0, 0.0895, -0.3749)` m.

The four source materials are `DarkWood`, `Metal`, `Grey`, and `LightMetal`. They contain dark linear PBR base colors; an unlit renderer should convert those colors for display or use suitable viewmodel lighting. The source material factors are preserved.

Original download SHA-256:

```text
3e661102b0c277625559316b6d5b6762536e70ac2b071fb9ef0f5f96f4d22158
```

Normalized runtime file SHA-256:

```text
8d1d696c551ae234892e7c102f0ba67b98a24be067bd8574216078a522b448da
```
