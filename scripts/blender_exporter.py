import bpy
import bmesh
import struct
import os
import bpy_extras.io_utils
from math import radians
from mathutils import Matrix
from mathutils import Euler
from mathutils import Vector
from mathutils import Quaternion
import numpy as np

    
#matrix for axis conversion (makes 'y' point up and 'z' forward)
M = Matrix(((1.0, 0.0, 0.0, 0.0),
            (0.0, 0.0, 1.0, 0.0),
            (0.0, 1.0, 0.0, 0.0),
            (0.0, 0.0, 0.0, 1.0)))

def convertQuat(quat):
    q = quat.copy()
    q.w, q.y, q.z = -q.w, q.z, q.y
    return q

def convertEuler(euler):
    return convertQuat(euler.to_quaternion())

#this only works if there's no scaling!!!
def convertMatrixNoScaling(matrix):
    trans = matrix.col[3].copy()
    trans.y, trans.z = trans.z, trans.y
    
    q = convertQuat(matrix.to_quaternion())
    
    T = q.to_matrix()
    T = T.to_4x4()
    T.col[3] = trans.copy()
    
    return T

#collision classes
class AABB:
    def __init__(self, min, max, verts):
        self.min = min
        self.max = max
        self.verts = verts
        
class BB:
    def __init__(self, verts):
        self.verts = verts
        
class Sphere:
    def __init__(self, center, radius):
        self.center = center
        self.radius = radius
        
#helper functions
def exportString(outfile, s):
    length = len(s)
    
    if length > 255:
        raise Exception("String exceeds 255 character limit!")
    
    outfile.write(length.to_bytes(1, byteorder='little', signed=False))
    
    if length > 0:
        outfile.write(bytearray(s, encoding='utf-8'))

def actionIsAnimation(action):
    return action.name != 'PoseLib' and action.frame_range[1] > 2.0

def actionIsPose(action):
    return action.name != 'PoseLib' and action.frame_range[1] <= 2.0

def exportAction(outfile, action, bones):
    if len(action.groups) == 0:
        raise Exception("Action has no action groups!")

    #verify that each action group (one for each bone) has 7 channels (3-pos, 4-rot_quat)
    #and that each channel of each action group has the same number of keyframes
    #and that the keyframes have the same timepoints
    for bone in bones:
        if action.groups.find(bone.name) == -1:
            raise Exception("No action group for bone " + bone.name + "!")
        
        action_group = action.groups[bone.name]
        if len(action_group.channels) != 7:
            raise Exception("Action group channel count is not 7!")
            
        for c in action_group.channels:
            if len(c.keyframe_points) != len(action.groups[0].channels[0].keyframe_points):
                raise Exception("Keyframe count mismatch!")
            
            for keyframe_point_id in range(0, len(c.keyframe_points)):
                if c.keyframe_points[keyframe_point_id].co[0] != action.groups[0].channels[0].keyframe_points[keyframe_point_id].co[0]:
                    raise Exception("Timepoint mismatch between keyframes!")
            
    exportString(outfile, action.name)

    #at this point we know that each channel in each action group has the same keyframes so we can grab timepoints from any channel in any group
    keyframe_points = [x.co[0] for x in action.groups[0].channels[0].keyframe_points]
    
    if actionIsAnimation(action):
        outfile.write(len(keyframe_points).to_bytes(1, byteorder='little', signed=False))
        for keyframe_point in keyframe_points:
            #TODO: read the 25.0 from scene/render settings to calculate correct timepoint
            t = keyframe_point / 25.0
            outfile.write(struct.pack('f', t))
        
    for bone in bones:
        action_group = action.groups[bone.name]
        
        for keyframe_point in keyframe_points:
            pos = [0.0, 0.0, 0.0]
            rot = [0.0, 0.0, 0.0, 0.0]
            
            for c in action_group.channels:
                if c.data_path.endswith("location"):
                    pos[c.array_index] = c.evaluate(keyframe_point)
                elif c.data_path.endswith("rotation_quaternion"):
                    rot[c.array_index] = c.evaluate(keyframe_point)
                else:
                    raise Exception("Wrong data_path in an fcurve!")
        
            q = convertQuat(Quaternion(rot))
            
            outfile.write(struct.pack('ffff', q[0], q[1], q[2], q[3]))
            outfile.write(struct.pack('fff', pos[0], pos[2], pos[1]))

def exportMaterial(outfile, object):
    color_texture = ""
    normal_map = ""
    
    if len(object.material_slots) > 1:
        raise Exception("Object has more than 1 material slot!")
    
    if len(object.material_slots) == 1:
        material = object.material_slots[0].material
        
        if material is not None:
            if material.node_tree:
                for node in material.node_tree.nodes:
                    if node.type == 'TEX_IMAGE':
                        for link in material.node_tree.links:
                            if link.from_node == node:
                                if link.to_node.type == 'NORMAL_MAP':
                                    normal_map = node.image
                                else:
                                    color_texture = node.image
                                break
            
    if not color_texture:
        exportString(outfile, "default.png")
    else:
        #full_path = bpy.path.abspath(tex.filepath, library=tex.library)
        #norm_path = os.path.normpath(full_path)
        path = bpy.path.basename(color_texture.filepath)
        exportString(outfile, path)
        
    if not normal_map:
        exportString(outfile, "")
    else:
        path = bpy.path.basename(normal_map.filepath)
        exportString(outfile, path)

def isRenderable(object):
    return (object.type == 'MESH') and ((object.parent is None) or (object.rigid_body is None) or (object.rigid_body.collision_shape == 'COMPOUND'))

def getObjectCollisionShapes(object, aabbs, bbs, spheres):
    bb_min = object.bound_box[0]
    bb_max = object.bound_box[0]
    for i in range(1, len(object.bound_box)):
        bb_min = np.minimum(object.bound_box[i], bb_min)
        bb_max = np.maximum(object.bound_box[i], bb_max)
        
    if object.parent is not None and object.parent.rigid_body is not None:
        T = M @ object.parent.matrix_world.inverted() @ object.matrix_world
    else:
        T = M
                
    bb_min = T @ Vector(bb_min)
    bb_max = T @ Vector(bb_max)
                
    if object.rigid_body.collision_shape == "BOX":
        verts = []
        for vert in object.bound_box:
            v = T @ Vector((vert[0], vert[1], vert[2]))
            verts.append(v[0])
            verts.append(v[1])
            verts.append(v[2])
        #if object has no rotation we can export its collision box as AABB
        if object.rotation_euler == Euler() and (object.parent is None or object.parent.rotation_euler == Euler()):
            aabbs.append(AABB(bb_min, bb_max, verts))
        else:
            bbs.append(BB(verts))
    elif object.rigid_body.collision_shape == "SPHERE":
        center = (bb_min + bb_max) / 2.0
        diagonal = bb_max - bb_min
        radius = max(diagonal) / 2.0
        spheres.append(Sphere(center, radius))
    elif object.rigid_body.collision_shape == "COMPOUND":
        for child in object.children:
            if child.rigid_body is not None:
                getObjectCollisionShapes(child, aabbs, bbs, spheres)
    
def exportObject(outfile, object):
    if isRenderable(object):
        me = object.data.copy()
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
    
        #bm.faces.ensure_lookup_table()
        #outfile.write(len(bm.faces).to_bytes(8, byteorder='little', signed=False))
        
        #ensure no world transform
        if any(object.location):
            raise Exception("Object has non-zero location!")
            
        if object.scale.x != 1 or object.scale.y != 1 or object.scale.z != 1:
            raise Exception("Object has non-unit scale!")
            
        if object.rotation_mode == 'QUATERNION':
            if object.rotation_quaternion != Quaternion():
                raise Exception("Object has non-zero rotation!")
        elif object.rotation_mode == 'AXIS_ANGLE':
            if object.axis_angle[0] != 0:
                raise Exception("Object has non-zero rotation!")
        elif any(object.rotation_euler):
            raise Exception("Object has non-zero rotation!")

        #material info
        exportMaterial(outfile, object)
        
        #vertex count
        outfile.write(len(me.loops).to_bytes(8, byteorder='little', signed=False))
        
        #for f in bm.faces:
        #    for v in f.verts:
        #        outfile.write(struct.pack('fff', v.co.x, v.co.z, v.co.y))
        #    outfile.write(struct.pack('fff', f.normal.x, f.normal.z, f.normal.y))
        uv_size_y = abs(me.uv_layers.active.data[0].uv.y - me.uv_layers.active.data[1].uv.y)

        me.calc_tangents()
        
        for poly in me.polygons:
            for index in poly.loop_indices:
                loop = me.loops[index]
                
                v = me.vertices[loop.vertex_index].co
                outfile.write(struct.pack('fff', v.x, v.z, v.y))
                
                n = loop.normal
                t = loop.tangent
                #b = loop.bitangent
                outfile.write(struct.pack('fff', n.x, n.z, n.y))
                outfile.write(struct.pack('fff', t.x, t.z, t.y))
                #outfile.write(struct.pack('fff', b.x, b.z, b.y))
                
                #blender uv coordinates have flipped y-axis compared to graphics APIs, hence "uv_size_y - uv.y"
                #this could also be changed at blender level if I knew how...
                uv = me.uv_layers.active.data[index].uv
                outfile.write(struct.pack('ff', uv.x, uv_size_y - uv.y))
                
                #vertex bone id
                #TODO: this is only for models with armatures - for now we have that with each vertex, even if no armature
                #but in the future there should be a separate model format for models with no animation
                if object.parent is not None and object.parent.type == 'ARMATURE':
                    armature = object.parent
                    bones = armature.data.bones
                    bone_count = len(bones)
                    
                    v = me.vertices[loop.vertex_index]
                    if len(v.groups) != 1:
                        raise Exception("Each vertex in a rigged mesh must belong to exactly one vertex group!")
                
                    vertex_group_id = v.groups[0].group
                    vertex_group_name = object.vertex_groups[vertex_group_id].name
                
                    for i in range(0, bone_count):
                        if bones[i].name == vertex_group_name:
                            outfile.write(i.to_bytes(4, byteorder='little', signed=False))
                            break
                else:
                    outfile.write((0).to_bytes(4, byteorder='little', signed=False))

#main export scene function
def exportScene(context, outfile):
    
    #export animations
    if len(bpy.data.armatures) > 1:
        raise Exception("More than 1 armature in the scene!")
        
    if len(bpy.data.armatures) == 1:
        armature = bpy.data.armatures[0]
        #bone count
        bones = armature.bones
        bone_count = len(bones)
        outfile.write(bone_count.to_bytes(1, byteorder='little', signed=False))
        
        if bone_count > 0:
            #bone parent ids
            for bone in bones:
                if bone.parent is None:
                    bone_parent_id = -1
                else:
                    for i in range(0, bone_count):
                        if bones[i].name == bone.parent.name:
                            bone_parent_id = i
                            break
                        
                outfile.write(bone_parent_id.to_bytes(1, byteorder='little', signed=True))
        
            #bone offset transforms
            for bone in bones:                
                bone_offset_T = convertMatrixNoScaling(bone.matrix_local).inverted()
                bone_offset_T.transpose()
                
                for vec in bone_offset_T:
                    outfile.write(struct.pack('ffff', *vec))
            
            pose_count = 0
            action_count = 0
            for action in bpy.data.actions:
                if actionIsAnimation(action):
                    action_count += 1
                elif actionIsPose(action):
                    pose_count += 1
            
            #write pose count 
            outfile.write(pose_count.to_bytes(1, byteorder='little', signed=False))
                            
            #poses
            for action in bpy.data.actions:
                if actionIsPose(action):
                    exportAction(outfile, action, bones)
            
            #write action count 
            outfile.write(action_count.to_bytes(1, byteorder='little', signed=False))
                            
            #animations
            for action in bpy.data.actions:
                if actionIsAnimation(action):
                    exportAction(outfile, action, bones)
    
    else:
        outfile.write((0).to_bytes(1, byteorder='little', signed=False))
        
    #export meshes
    obj_count = 0
    
    for object in context.scene.objects:
        if isRenderable(object):
            obj_count += 1
    
    outfile.write(obj_count.to_bytes(8, byteorder='little', signed=False))
    
    for object in context.scene.objects:
         exportObject(outfile, object)
    
    #export collision data
    object = context.scene.objects[0]
    #bound sphere (frustum culling)
    bb_min = object.bound_box[0]
    bb_max = object.bound_box[0]
    for i in range(1, len(object.bound_box)):
        bb_min = np.minimum(object.bound_box[i], bb_min)
        bb_max = np.maximum(object.bound_box[i], bb_max)
            
    center = (bb_min + bb_max) / 2.0
    diagonal = bb_max - bb_min
    radius = max(diagonal) / 2.0
            
    outfile.write(struct.pack('fff', center[0], center[2], center[1]))
    outfile.write(struct.pack('f', radius))
                    
    #collision
    aabbs = []
    bbs = []
    spheres = []
        
    if object.rigid_body:
        getObjectCollisionShapes(object, aabbs, bbs, spheres)
        
    outfile.write((len(aabbs)).to_bytes(1, byteorder='little', signed=False))
    for aabb in aabbs:
        outfile.write(struct.pack('fff', aabb.min[0], aabb.min[1], aabb.min[2]))
        outfile.write(struct.pack('fff', aabb.max[0], aabb.max[1], aabb.max[2]))
        outfile.write(struct.pack('%sf' % len(aabb.verts), *aabb.verts))
    
    outfile.write((len(bbs)).to_bytes(1, byteorder='little', signed=False))
    for bb in bbs:
        outfile.write(struct.pack('%sf' % len(bb.verts), *bb.verts))
        outfile.write(struct.pack('fff', 1.0, 0.0, 0.0))
        outfile.write(struct.pack('fff', 0.0, 1.0, 0.0))
        outfile.write(struct.pack('fff', 0.0, 0.0, 1.0))
    
    outfile.write((len(spheres)).to_bytes(1, byteorder='little', signed=False))
    for s in spheres:
        outfile.write(struct.pack('fff', s.center[0], s.center[1], s.center[2]))
        outfile.write(struct.pack('f', s.radius))
    
    outfile.close()

    
# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """My exporter"""
    bl_idname = "export_test.some_data"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export"

    # ExportHelper mixin class uses this
    filename_ext = ".3d"

    filter_glob: StringProperty(
        default="*.3d",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting: BoolProperty(
        name="Example Boolean",
        description="Example Tooltip",
        default=True,
    )

    #type: EnumProperty(
    #    name="Export",
    #    description="Choose between two items",
    #    items=(
    #        ('SCENE', "Scene", "Exports scene as a scene."),
    #        ('OBJECT', "Object", "Exports scene as an object."),
    #    ),
    #    default='SCENE',
    #)

    def execute(self, context):
        outfile = open(self.filepath, 'wb') 
        exportScene(context, outfile)
            
        return {'FINISHED'}


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="My Exporter")


def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')
