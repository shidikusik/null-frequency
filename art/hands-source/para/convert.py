"""Convert the CC0 OpenGameArt para FPS arms into a deform-only GLB.
Run: blender --background --disable-autoexec --python this_file.py
"""
import bpy,json
from pathlib import Path
from mathutils import Matrix
base=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(base/'FPS ARMS RIG 1.blend'))
if bpy.context.object and bpy.context.object.mode!='OBJECT':
 bpy.ops.object.mode_set(mode='OBJECT')
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
mesh=next(o for o in bpy.data.objects if o.type=='MESH')
rig.animation_data_clear()
for bone in rig.pose.bones:
 bone.matrix_basis=Matrix.Identity(4)
 for constraint in list(bone.constraints):bone.constraints.remove(constraint)
bpy.ops.object.select_all(action='DESELECT')
rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT')
for side in ('L','R'):
 hand=rig.data.edit_bones['hand.'+side]
 hand.parent=rig.data.edit_bones['forearm.'+side]
 hand.use_connect=True
for bone in list(rig.data.edit_bones):
 if '.control' in bone.name:rig.data.edit_bones.remove(bone)
bpy.ops.object.mode_set(mode='OBJECT')
rig.data.transform(Matrix.Scale(.1,4))
rig.data.pose_position='REST'
# Convert the rest mesh and skeleton together into metres, avoiding scaled
# skin-parent nodes, which older glTF runtimes can apply twice.
bpy.context.scene.unit_settings.scale_length=1.0
for vertex in mesh.data.vertices:vertex.co*=.1
rig.location*=.1
mesh.location*=.1
bpy.context.view_layer.update()
world=rig.matrix_world.copy()
rig.data.transform(world)
mesh.data.transform(world)
rig.matrix_world=Matrix.Identity(4)
mesh.matrix_parent_inverse=Matrix.Identity(4)
mesh.matrix_basis=Matrix.Identity(4)
image=bpy.data.images.load(str(base/'new_diff.png'),check_existing=True)
mat=bpy.data.materials.new('para_skin_CC0');mat.use_nodes=True
nodes=mat.node_tree.nodes;bsdf=nodes.get('Principled BSDF')
bsdf.inputs['Roughness'].default_value=.78
tex=nodes.new('ShaderNodeTexImage');tex.image=image
mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
mesh.data.materials.clear();mesh.data.materials.append(mat)
bpy.ops.object.select_all(action='DESELECT')
for obj in (rig,mesh):obj.select_set(True)
bpy.context.view_layer.objects.active=mesh
bpy.ops.export_scene.gltf(filepath=str(base/'para.glb'),export_format='GLB',use_selection=True,export_animations=False,export_skins=True,export_def_bones=True,export_yup=True,export_materials='EXPORT',export_image_format='AUTO')
print('PARA_EXPORTED',base/'para.glb')
