# Third-party notices

The original game code is distributed under the MIT license in `LICENSE`.

| Component | Source | License |
| --- | --- | --- |
| raylib 5.5 | <https://github.com/raysan5/raylib/tree/5.5> | zlib/libpng license; a copy is included as `licenses/raylib-LICENSE.txt` in binary packages. Source builds retain it in the downloaded dependency. |
| Noto Sans | <https://github.com/notofonts/latin-greek-cyrillic> | SIL Open Font License 1.1; see `assets/fonts/NotoSans-OFL.txt`. Copyright 2022 The Noto Project Authors. |
| Oswald | <https://github.com/google/fonts/tree/main/ofl/oswald> | SIL Open Font License 1.1; see `assets/fonts/Oswald-OFL.txt`. |

raylib includes its own third-party components. Their complete source notices
are retained in the [raylib source](https://github.com/raysan5/raylib/tree/5.5/src/external).
Notices that apply to the binary packages are reproduced below.

The raylib 5.5 source archive used by both build systems is verified against:

```text
aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
```

The game uses redistributable 3D assets alongside its procedural architecture,
effects and soundscape. Original asset sources, authors, licenses and runtime
changes are recorded beside the files:

- [Quaternius revolver](assets/models/weapon/SOURCE.md), CC0 1.0.
- [First-person arms by para, with mesh and texture from MakeHuman](assets/models/hands/SOURCE.md), CC0 1.0.
- [Quaternius animated humanoid](assets/models/entity/SOURCE.md), CC0 1.0.
- [Poly Haven industrial props](assets/models/props/SOURCE.md), CC0 1.0.
- [Poly Haven concrete textures](assets/textures/SOURCE.md), CC0 1.0.

These source and license files are included in the binary packages. No recorded
music or sound effects are downloaded; all audio is synthesized by the game.

The repository also preserves editable Blender/FBX source files and an unused
alternative arms model by DevMops under [`art/hands-source`](art/hands-source/SOURCE.md).
Both source sets are CC0 1.0; they are excluded from binary packages.

## MIT-licensed components in raylib

The following copyright notices accompany the MIT license text below:

```text
tinyobj_loader_c: Copyright (c) 2016 - 2019 Syoyo Fujita and many contributors.
vox_loader: Copyright (c) 2021 Johann Nadalutti.
cgltf: Copyright (c) 2018-2021 Johannes Kuhlmann
jsmn (included in cgltf): Copyright (c) 2010 Serge A. Zaitsev
par_shapes: Copyright (c) 2019 Philip Rideout
m3d: Copyright (C) 2020 bzt (bztsrc@gitlab)
qoi: Copyright (c) 2021, Dominic Szablewski - https://phoboslab.org
qoa: Copyright (c) 2023, Dominic Szablewski - https://phoboslab.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Khronos OpenGL platform headers

```text
Copyright (c) 2008-2018 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Materials.

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
```

## Windows directory compatibility code

```text
Copyright Kevlin Henney, 1997, 2003. All rights reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose is hereby granted without fee, provided
that this copyright and permissions notice appear in all copies and
derivatives.

This software is supplied "as is" without express or implied warranty.
```

## Other bundled components

GLFW is copyright (c) 2002-2006 Marcus Geelnard and (c) 2006-2019 Camilla
Löwy, under the zlib/libpng license; its full license is included in binary
packages as `licenses/GLFW-LICENSE.txt`.

miniaudio and the dr_* decoders by David Reid are used under their offered
public-domain / Unlicense option. The stb libraries by Sean Barrett and
contributors and sdefl/sinfl by Micha Mettke are used under their offered
public-domain option. jar_xm and jar_mod are provided under WTFPL version 2.
raylib's rprand and rl_gputex helpers use the zlib/libpng license.
