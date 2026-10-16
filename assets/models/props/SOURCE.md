# Environment model sources

Downloaded from the official Poly Haven public API and file CDN on
2026-10-15. All three models are **CC0 1.0 Universal** assets; see the
[Poly Haven asset license](https://polyhaven.com/license), the
[CC0 summary](https://creativecommons.org/publicdomain/zero/1.0/) and the
included [CC0 legal text](CC0-1.0.txt).

| Local glTF | Original asset | Author | Triangles |
| --- | --- | --- | ---: |
| `utility/utility.gltf` | [Utility Box 02](https://polyhaven.com/a/utility_box_02) | James Ray Cock | 6,268 |
| `barrel/barrel.gltf` | [Barrel 03](https://polyhaven.com/a/barrel_03) | Serhii Khromov | 1,473 |
| `chair/chair.gltf` | [Wheelchair 01](https://polyhaven.com/a/wheelchair_01) | Garreth Dean | 40,252 |

The files are the official uncompressed glTF 2.0 downloads with 1K JPEG
materials, external geometry buffers, diffuse maps, OpenGL normal maps and
packed AO/roughness/metallic maps. Only the top-level `.gltf` filenames have
been renamed. Geometry, UVs, materials, map references and texture images are
unchanged. There is no Draco or other required glTF extension.

The original download lists (including URLs, byte sizes and MD5 checksums)
are available from the official API:

- [Utility Box 02 files](https://api.polyhaven.com/files/utility_box_02)
- [Barrel 03 files](https://api.polyhaven.com/files/barrel_03)
- [Wheelchair 01 files](https://api.polyhaven.com/files/wheelchair_01)

Every downloaded model, geometry buffer and texture was verified against
the corresponding API MD5 checksum. All glTF image/buffer references resolve
to local files. raylib 5.5 successfully loaded each model and its 1024 × 1024
diffuse map in a hidden OpenGL window.

## Integration measurements

The models are already upright on the Y axis and use metres. They each have
one mesh and one authored material. raylib also creates its default material,
so `materialCount == 2` and the mesh uses material index 1. Use
`meshMaterial[meshIndex]` when accessing the diffuse texture.

| Model | Minimum `(x, y, z)` | Maximum `(x, y, z)` | Height |
| --- | --- | --- | ---: |
| Utility box | `(-0.4600, 0.0000, -0.2100)` | `(0.4600, 1.1203, 0.2220)` | 1.1203 m |
| Barrel | `(-0.3170, 0.0000, -0.3193)` | `(0.3170, 0.9305, 0.3193)` | 0.9305 m |
| Wheelchair | `(-0.4137, 0.0058, -0.4857)` | `(0.4104, 1.1093, 0.6052)` | 1.1035 m |

The authored glTF materials are marked double-sided. Geometry is static;
these prop files contain no animation clips.

The accompanying world surface textures are documented in
[`assets/textures/SOURCE.md`](../../textures/SOURCE.md).
