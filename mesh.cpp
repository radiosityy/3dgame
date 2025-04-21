#include "mesh.h"
#include <fstream>
#include "game_utils.h"

AnimatedMeshData readAnimatedMeshData(std::ifstream& mesh_file, uint8_t bone_count)
{
    AnimatedMeshData anim_data;

    anim_data.bone_count = bone_count;
    anim_data.bone_parent_ids.resize(anim_data.bone_count);
    mesh_file.read(reinterpret_cast<char*>(anim_data.bone_parent_ids.data()), anim_data.bone_count * sizeof(int8_t));

    anim_data.bone_offset_transforms.resize(anim_data.bone_count);
    mesh_file.read(reinterpret_cast<char*>(anim_data.bone_offset_transforms.data()), anim_data.bone_count * sizeof(mat4x4));

    anim_data.bone_to_root_transforms.resize(anim_data.bone_count);
    for(uint32_t bone_id = 0; bone_id < anim_data.bone_count; bone_id++)
    {
        anim_data.bone_to_root_transforms[bone_id] = inverse(anim_data.bone_offset_transforms[bone_id]);
    }

    uint8_t pose_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&pose_count), sizeof(uint8_t));

    for(uint8_t pose = 0; pose < pose_count; pose++)
    {
        uint8_t name_length;
        mesh_file.read(reinterpret_cast<char*>(&name_length), sizeof(uint8_t));
        std::string name(name_length, '\0');
        mesh_file.read(reinterpret_cast<char*>(name.data()), name_length);

        anim_data.poses[name.c_str()].resize(anim_data.bone_count);
        mesh_file.read(reinterpret_cast<char*>(anim_data.poses[name.c_str()].data()), anim_data.bone_count * sizeof(KeyFrame));
    }

    uint8_t animation_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&animation_count), sizeof(uint8_t));

    for(uint8_t anim = 0; anim < animation_count; anim++)
    {
        uint8_t name_length;
        mesh_file.read(reinterpret_cast<char*>(&name_length), sizeof(uint8_t));
        std::string name(name_length, '\0');
        mesh_file.read(reinterpret_cast<char*>(name.data()), name_length);

        uint8_t keyframe_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&keyframe_count), sizeof(uint8_t));
        anim_data.animations[name.c_str()].key_frame_times.resize(keyframe_count);
        mesh_file.read(reinterpret_cast<char*>(anim_data.animations[name.c_str()].key_frame_times.data()), keyframe_count * sizeof(float));

        anim_data.animations[name.c_str()].key_frames.resize(anim_data.bone_count);

        for(auto& key_frames : anim_data.animations[name.c_str()].key_frames)
        {
            key_frames.resize(keyframe_count);
            mesh_file.read(reinterpret_cast<char*>(key_frames.data()), keyframe_count * sizeof(KeyFrame));
        }
    }

    return anim_data;
}

MeshData MeshManager::readMeshData(Renderer& renderer, std::ifstream& mesh_file)
{
    MeshData mesh_data;

    uint8_t texture_filename_length = 0;
    mesh_file.read(reinterpret_cast<char*>(&texture_filename_length), sizeof(uint8_t));

    if(texture_filename_length > 0)
    {
        std::string texture_filename;
        texture_filename.resize(texture_filename_length);
        mesh_file.read(reinterpret_cast<char*>(texture_filename.data()), texture_filename_length);
        mesh_data.tex_id = renderer.loadTexture(texture_filename);
    }

    uint8_t normal_map_filename_length = 0;
    mesh_file.read(reinterpret_cast<char*>(&normal_map_filename_length), sizeof(uint8_t));

    if(normal_map_filename_length > 0)
    {
        std::string normal_map_filename;
        normal_map_filename.resize(normal_map_filename_length);
        mesh_file.read(reinterpret_cast<char*>(normal_map_filename.data()), normal_map_filename_length);
        mesh_data.normal_map_id = renderer.loadNormalMap(normal_map_filename);
    }

    uint64_t vertex_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint64_t));

    mesh_data.vertex_data.resize(vertex_count);
    mesh_file.read(reinterpret_cast<char*>(mesh_data.vertex_data.data()), vertex_count * sizeof(VertexDefault));

    mesh_data.vb_alloc = renderer.reqVBAlloc<VertexDefault>(vertex_count);
    renderer.updateVertexData(mesh_data.vb_alloc.vb, mesh_data.vb_alloc.data_offset, sizeof(VertexDefault) * vertex_count, mesh_data.vertex_data.data());
#if EDITOR_ENABLE
    m_mesh_data_size += vertex_count * sizeof(VertexDefault);
#endif

    /*------ TODO: REMOVE AFTER REMOVED FROM EXPORT SCRIPT --------*/
    Sphere bounding_sphere;
    mesh_file.read(reinterpret_cast<char*>(&bounding_sphere), sizeof(Sphere));

    uint8_t aabb_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&aabb_count), sizeof(uint8_t));
    std::vector<AABB> aabbs(aabb_count);
    mesh_file.read(reinterpret_cast<char*>(aabbs.data()), aabb_count * sizeof(AABB));

    uint8_t bb_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&bb_count), sizeof(uint8_t));
    std::vector<BoundingBox> bbs(bb_count);
    mesh_file.read(reinterpret_cast<char*>(bbs.data()), bb_count * sizeof(BoundingBox));

    uint8_t sphere_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&sphere_count), sizeof(uint8_t));
    std::vector<Sphere> spheres(sphere_count);
    mesh_file.read(reinterpret_cast<char*>(spheres.data()), sphere_count * sizeof(Sphere));

    /*-------------------------------------------------------------*/

    return mesh_data;
}

void MeshManager::loadMeshData(Renderer& renderer, std::string_view mesh_filename)
{
    std::ifstream mesh_file(mesh_filename.data(), std::ios::binary);

    if(!mesh_file)
    {
        error(std::format("Failed to open mesh file: {}", mesh_filename));
    }

    uint8_t bone_count = 0;
    mesh_file.read(reinterpret_cast<char*>(&bone_count), sizeof(uint8_t));

    if(bone_count != 0)
    {
        auto anim_mesh_data = readAnimatedMeshData(mesh_file, bone_count);
        auto mesh_data = readMeshData(renderer, mesh_file);
        m_animated_mesh_data.emplace(std::move(mesh_data), std::move(anim_mesh_data));
    }
    else
    {
        m_mesh_data.emplace(readMeshData(renderer, mesh_file));
    }


#if EDITOR_ENABLE
    m_mesh_filenames.emplace_back(mesh_filename);
#endif
}

Mesh MeshManager::mesh(std::string_view mesh_name) const
{
    return Mesh(&m_mesh_data.at(mesh_name.data()));
}

AnimatedMesh MeshManager::animatedMesh(Renderer& renderer, std::string_view mesh_name) const
{
    const auto& [mesh_data, anim_mesh_data] = m_animated_mesh_data.at(mesh_name.data());
    return AnimatedMesh(renderer, &mesh_data, &anim_mesh_data);
}

uint64_t MeshManager::meshDataSize() const
{
    return m_mesh_data_size;
}

const std::vector<std::string>& MeshManager::meshFilenames() const
{
    return m_mesh_filenames;
}

uint32_t Mesh::textureId() const
{
    return m_mesh_data->tex_id;
}

uint32_t Mesh::normalMapId() const
{
    return m_mesh_data->normal_map_id;
}

VertexBuffer* Mesh::vertexBuffer() const
{
    return m_mesh_data->vb_alloc.vb;
}

uint32_t Mesh::vertexBufferOffset() const
{
    return m_mesh_data->vb_alloc.vertex_offset;
}

uint32_t Mesh::vertexCount() const
{
    return m_mesh_data->vertex_data.size();
}

const std::vector<VertexDefault>& Mesh::vertexData() const
{
    return m_mesh_data->vertex_data;
}

AnimatedMesh::AnimatedMesh(Renderer& renderer, const MeshData* mesh_data, const AnimatedMeshData* anim_mesh_data)
    : Mesh(mesh_data)
    , m_animated_mesh_data(anim_mesh_data)
{
    m_src_key_frames.resize(m_animated_mesh_data->bone_count, nullptr);
    m_dst_key_frames.resize(m_animated_mesh_data->bone_count, nullptr);
    m_curr_pose.resize(m_animated_mesh_data->bone_count);
    m_bone_transforms.resize(m_animated_mesh_data->bone_count);
    resetPose();
    m_bone_offset = renderer.reqBoneBufAlloc(m_animated_mesh_data->bone_count);
    renderer.updateBoneTransformData(m_bone_offset, m_animated_mesh_data->bone_count, m_bone_transforms.data());
}

uint32_t AnimatedMesh::boneOffset() const
{
    return m_bone_offset;
}

void AnimatedMesh::resetPose()
{
    if(m_animated_mesh_data->poses.empty())
    {
        for(auto& T : m_bone_transforms)
        {
            T = glm::identity<mat4x4>();
        }
    }
    else
    {
        setPose("pose_default");
    }
}

void AnimatedMesh::stopAnimation()
{
    animateToPose("pose_default");
}

void AnimatedMesh::animateToPose(std::string_view pose_name)
{
    m_play_animation = true;
    m_loop_animation = false;
    m_continue_animation = false;

    m_interrupt_pose = m_curr_pose;

    for(uint32_t bone_id = 0; bone_id < m_animated_mesh_data->bone_count; bone_id++)
    {
        m_src_key_frames[bone_id] = &m_interrupt_pose[bone_id];
        m_dst_key_frames[bone_id] = &m_animated_mesh_data->poses.at(pose_name.data())[bone_id];
    }

    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;
}

void AnimatedMesh::setAnimKeyframe(uint8_t keyframe)
{
    m_curr_anim_keyframe = keyframe;
    for(uint8_t i = 0; i < m_animated_mesh_data->bone_count; i++)
    {
        m_src_key_frames[i] = &m_curr_anim->key_frames[i][m_curr_anim_keyframe];
        m_dst_key_frames[i] = &m_curr_anim->key_frames[i][m_curr_anim_keyframe + 1];
    }

    if(m_key_frame_elapsed_time > 0.0f)
    {
        m_key_frame_elapsed_time -= m_key_frame_time;
    }

    m_key_frame_time = m_curr_anim->key_frame_times[m_curr_anim_keyframe + 1] - m_curr_anim->key_frame_times[m_curr_anim_keyframe];
}

void AnimatedMesh::playAnimation(std::string_view name)
{
    m_curr_anim = &m_animated_mesh_data->animations.at(name.data());
    m_curr_anim_keyframe = -1;
    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;

    m_interrupt_pose = m_curr_pose;

    for(uint8_t i = 0; i < m_animated_mesh_data->bone_count; i++)
    {
        m_src_key_frames[i] = &m_interrupt_pose[i];
        m_dst_key_frames[i] = &m_curr_anim->key_frames[i][0];
    }

    m_play_animation = true;
    m_continue_animation = true;
    m_loop_animation = true;
}

void AnimatedMesh::setPose(std::string_view pose_name)
{
    const Pose& pose = m_animated_mesh_data->poses.at(pose_name.data());

    for(uint32_t bone_id = 0; bone_id < m_animated_mesh_data->bone_count; bone_id++)
    {
        const auto q = pose[bone_id].rot;
        const auto trans = glm::translate(pose[bone_id].pos);

        const auto rot = mat4_cast(q);
        auto T = m_animated_mesh_data->bone_to_root_transforms[bone_id] * trans * rot * m_animated_mesh_data->bone_offset_transforms[bone_id];

        m_bone_transforms[bone_id] = T;

        if(m_animated_mesh_data->bone_parent_ids[bone_id] != -1)
        {
            m_bone_transforms[bone_id] = m_bone_transforms[m_animated_mesh_data->bone_parent_ids[bone_id]] * m_bone_transforms[bone_id];
        }
    }

    m_play_animation = false;
    m_curr_pose = m_animated_mesh_data->poses.at(pose_name.data());
}

void AnimatedMesh::animationUpdate(Renderer& renderer, float dt)
{
    if(m_animated_mesh_data->bone_count != 0)
    {
        if(m_play_animation)
        {
            const float t = m_key_frame_elapsed_time / m_key_frame_time;

            for(uint32_t bone_id = 0; bone_id < m_animated_mesh_data->bone_count; bone_id++)
            {
                const KeyFrame* src_key_frame = m_src_key_frames[bone_id];
                const KeyFrame* dst_key_frame = m_dst_key_frames[bone_id];

                m_curr_pose[bone_id].rot = glm::slerp(src_key_frame->rot, dst_key_frame->rot, t);
                m_curr_pose[bone_id].pos  = glm::mix(src_key_frame->pos, dst_key_frame->pos, t);

                const auto rot = mat4_cast(m_curr_pose[bone_id].rot);
                const auto trans = glm::translate(m_curr_pose[bone_id].pos);

                auto T = m_animated_mesh_data->bone_to_root_transforms[bone_id] * trans * rot * m_animated_mesh_data->bone_offset_transforms[bone_id];

                m_bone_transforms[bone_id] = T;

                if(m_animated_mesh_data->bone_parent_ids[bone_id] != -1)
                {
                    m_bone_transforms[bone_id] = m_bone_transforms[static_cast<uint8_t>(m_animated_mesh_data->bone_parent_ids[bone_id])] * m_bone_transforms[bone_id];
                }
            }

            m_key_frame_elapsed_time += dt;

            if(m_key_frame_elapsed_time > m_key_frame_time)
            {
                if(m_continue_animation)
                {
                    if(m_curr_anim_keyframe + 1u == m_curr_anim->key_frame_times.size() - 1u)
                    {
                        if(m_loop_animation)
                        {
                            setAnimKeyframe(0);
                        }
                        else
                        {
                            stopAnimation();
                            return;
                        }
                    }
                    else
                    {
                        setAnimKeyframe(m_curr_anim_keyframe + 1);
                    }
                }
                else
                {
                    m_play_animation = false;
                }
            }
        }

        //TODO: only update if data changed
        renderer.updateBoneTransformData(m_bone_offset, m_animated_mesh_data->bone_count, m_bone_transforms.data());
    }
}

bool AnimatedMesh::forPlayer1(std::string_view anim_name)
{
    return !m_play_animation || (m_curr_anim != &m_animated_mesh_data->animations.at(anim_name.data())) || !m_continue_animation;
}

bool AnimatedMesh::forPlayer2(std::string_view anim_name)
{
    return m_curr_anim == &m_animated_mesh_data->animations.at(anim_name.data());
}
