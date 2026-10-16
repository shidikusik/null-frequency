#!/usr/bin/env python3
"""Pack Quaternius's CC0 animation-library humanoid into a compact GLB.

Usage: python prepare_entity.py /path/AnimationLibrary_Godot_Standard.gltf entity.glb
The matching .bin must be beside the input .gltf. No third-party Python modules.
"""
import copy
import json
import pathlib
import struct
import sys

source = pathlib.Path(sys.argv[1])
destination = pathlib.Path(sys.argv[2])
document = json.loads(source.read_text())
buffers = [(source.parent / item["uri"]).read_bytes() for item in document["buffers"]]
selected = ["Idle_Loop", "Walk_Loop", "Punch_Cross", "Punch_Jab", "Hit_Chest", "Death01"]
by_name = {animation["name"]: animation for animation in document["animations"]}
document["animations"] = [copy.deepcopy(by_name[name]) for name in selected]

used_accessors = set()
for mesh in document["meshes"]:
    for primitive in mesh["primitives"]:
        used_accessors.update(primitive["attributes"].values())
        if "indices" in primitive:
            used_accessors.add(primitive["indices"])
for skin in document["skins"]:
    used_accessors.add(skin["inverseBindMatrices"])
for animation in document["animations"]:
    for sampler in animation["samplers"]:
        used_accessors.update((sampler["input"], sampler["output"]))

accessor_indices = {old: new for new, old in enumerate(sorted(used_accessors))}
accessors = [copy.deepcopy(document["accessors"][old]) for old in sorted(used_accessors)]
used_views = sorted({accessor["bufferView"] for accessor in accessors})
view_indices = {old: new for new, old in enumerate(used_views)}
views = []
binary = bytearray()
for old_index in used_views:
    view = copy.deepcopy(document["bufferViews"][old_index])
    binary.extend(b"\0" * ((-len(binary)) % 4))
    offset = view.get("byteOffset", 0)
    data = buffers[view["buffer"]][offset:offset + view["byteLength"]]
    view.update(buffer=0, byteOffset=len(binary))
    binary.extend(data)
    views.append(view)
for accessor in accessors:
    accessor["bufferView"] = view_indices[accessor["bufferView"]]
for mesh in document["meshes"]:
    for primitive in mesh["primitives"]:
        primitive["attributes"] = {key: accessor_indices[value] for key, value in primitive["attributes"].items()}
        if "indices" in primitive:
            primitive["indices"] = accessor_indices[primitive["indices"]]
for skin in document["skins"]:
    skin["inverseBindMatrices"] = accessor_indices[skin["inverseBindMatrices"]]
for animation in document["animations"]:
    for sampler in animation["samplers"]:
        sampler["input"] = accessor_indices[sampler["input"]]
        sampler["output"] = accessor_indices[sampler["output"]]

# The source uses orange and purple; neutral mineral tones suit the abandoned
# acoustic station and preserve the original anatomical detail and joint shapes.
for index, material in enumerate(document["materials"]):
    material["pbrMetallicRoughness"] = {
        "baseColorFactor": [.56, .58, .55, 1] if index == 0 else [.14, .17, .16, 1],
        "metallicFactor": .08,
        "roughnessFactor": .82,
    }

document["accessors"] = accessors
document["bufferViews"] = views
document["buffers"] = [{"byteLength": len(binary)}]
document["asset"]["copyright"] = "Quaternius, Universal Animation Library, CC0 1.0"
document["asset"]["generator"] = "NULL FREQUENCY prepare_entity.py; source mesh and skeletal clips by Quaternius"
json_chunk = json.dumps(document, separators=(",", ":")).encode("utf-8")
json_chunk += b" " * ((-len(json_chunk)) % 4)
binary.extend(b"\0" * ((-len(binary)) % 4))
glb = struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(json_chunk) + 8 + len(binary))
glb += struct.pack("<II", len(json_chunk), 0x4E4F534A) + json_chunk
glb += struct.pack("<II", len(binary), 0x004E4942) + binary
destination.write_bytes(glb)
print(f"Wrote {destination}: {len(glb):,} bytes; clips: {', '.join(selected)}")
