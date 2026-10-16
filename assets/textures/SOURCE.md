# World surface texture sources

Downloaded from the official Poly Haven API/file CDN on 2026-10-15.
All maps are **CC0 1.0 Universal** assets under the
[Poly Haven asset license](https://polyhaven.com/license).
The [full CC0 legal text](CC0-1.0.txt) is included in this folder.

| File | Original asset | Authors | Resolution | Physical tile size |
| --- | --- | --- | --- | --- |
| `wall.jpg`, `wall-normal.jpg` | [Cracked Concrete Wall](https://polyhaven.com/a/cracked_concrete_wall) | Dimitrios Savva: photography/processing; Rico Cilliers: guidance | 1024 × 1024 | 1 × 1 m |
| `floor.jpg`, `floor-normal.jpg` | [Concrete Floor Worn 001](https://polyhaven.com/a/concrete_floor_worn_001) | Dimitrios Savva: photography; Rico Cilliers: processing | 1024 × 1024 | 3 × 3 m |

These are the official 1K JPEG diffuse/albedo and OpenGL tangent-space normal
downloads (`nor_gl`, positive Y / green). Images are unchanged; only local
filenames were shortened for loading by the game. Use repeating texture
wrapping when tiling walls or floors. The `*-normal.jpg` files are linear
surface normals, not colour/albedo maps.

Official download metadata:

- [Cracked Concrete Wall files](https://api.polyhaven.com/files/cracked_concrete_wall)
- [Concrete Floor Worn 001 files](https://api.polyhaven.com/files/concrete_floor_worn_001)

Downloads were verified against the official API MD5 checksums:

```text
46d9751f0dcd35ea8f40962a577b7ece  wall.jpg
e35597cca586150b1ab2aa9a331a39c5  floor.jpg
1a3db636c2527165a8902f08dc13a2ee  wall-normal.jpg
9de6626758f8793b71182b892c110827  floor-normal.jpg
```
