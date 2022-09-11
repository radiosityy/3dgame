#include "mesh.h"
#include <fstream>

Mesh::Mesh(Engine3D& engine3d, std::string_view filename)
{
    if(m_mesh_datas.contains(filename.data()))
    {
        m_mesh_data = m_mesh_datas[filename.data()];
    }
    else
    {
        MeshData* mesh_data = new MeshData;

        std::ifstream mesh_file(filename.data(), std::ios::binary);

        if(!mesh_file)
        {
            throw std::runtime_error(std::string("Failed to open mesh file: ") + filename.data());
        }

        uint64_t obj_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&obj_count), sizeof(uint64_t));
        if(obj_count != 1)
        {
            throw std::runtime_error("Object count in an object file is " + std::to_string(obj_count) + "! Must be 1!");
        }

        uint64_t vertex_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint64_t));

        mesh_data->vb_alloc = engine3d.requestVertexBufferAllocation<VertexDefault>(vertex_count);
        mesh_data->vertex_data.resize(vertex_count);

        //TODO: read the vertex data in a single read() rather than in a loop
        for(uint64_t i = 0; i < vertex_count; i++)
        {
            mesh_file.read(reinterpret_cast<char*>(&mesh_data->vertex_data[i].pos), sizeof(vec3));
            mesh_file.read(reinterpret_cast<char*>(&mesh_data->vertex_data[i].normal), sizeof(vec3));
            mesh_file.read(reinterpret_cast<char*>(&mesh_data->vertex_data[i].tangent), sizeof(vec3));
            mesh_file.read(reinterpret_cast<char*>(&mesh_data->vertex_data[i].tex_coords), sizeof(vec2));
        }

        uint8_t texture_filename_length = 0;
        mesh_file.read(reinterpret_cast<char*>(&texture_filename_length), sizeof(uint8_t));

        if(texture_filename_length > 0)
        {
            std::string texture_filename;
            texture_filename.resize(texture_filename_length);
            mesh_file.read(reinterpret_cast<char*>(texture_filename.data()), texture_filename_length);
            mesh_data->tex_id = engine3d.loadTexture(texture_filename);
        }

        uint8_t normal_map_filename_length = 0;
        mesh_file.read(reinterpret_cast<char*>(&normal_map_filename_length), sizeof(uint8_t));

        if(normal_map_filename_length > 0)
        {
            std::string normal_map_filename;
            normal_map_filename.resize(normal_map_filename_length);
            mesh_file.read(reinterpret_cast<char*>(normal_map_filename.data()), normal_map_filename_length);
            mesh_data->normal_map_id = engine3d.loadNormalMap(normal_map_filename);
        }

        mesh_file.read(reinterpret_cast<char*>(&mesh_data->bounding_sphere), sizeof(Sphere));

        uint8_t aabb_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&aabb_count), sizeof(uint8_t));
        mesh_data->aabbs.resize(aabb_count);
        mesh_file.read(reinterpret_cast<char*>(mesh_data->aabbs.data()), aabb_count * sizeof(AABB));

        uint8_t bb_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&bb_count), sizeof(uint8_t));
        mesh_data->bbs.resize(bb_count);
        mesh_file.read(reinterpret_cast<char*>(mesh_data->bbs.data()), bb_count * sizeof(BoundingBox));

        uint8_t sphere_count = 0;
        mesh_file.read(reinterpret_cast<char*>(&sphere_count), sizeof(uint8_t));
        mesh_data->spheres.resize(sphere_count);
        mesh_file.read(reinterpret_cast<char*>(mesh_data->spheres.data()), sphere_count * sizeof(Sphere));

        mesh_file.read(reinterpret_cast<char*>(&mesh_data->bone_count), sizeof(uint8_t));

        if(mesh_data->bone_count != 0)
        {
            mesh_data->bone_offset = engine3d.requestBoneTransformBufferAllocation(mesh_data->bone_count);

            mesh_data->bone_parent_ids.resize(mesh_data->bone_count);
            mesh_file.read(reinterpret_cast<char*>(mesh_data->bone_parent_ids.data()), mesh_data->bone_count * sizeof(int8_t));

            mesh_data->bone_offset_transforms.resize(mesh_data->bone_count);
            mesh_file.read(reinterpret_cast<char*>(mesh_data->bone_offset_transforms.data()), mesh_data->bone_count * sizeof(mat4x4));

            mesh_data->bone_to_root_transforms.resize(mesh_data->bone_count);
            for(uint32_t bone_id = 0; bone_id < mesh_data->bone_count; bone_id++)
            {
                mesh_data->bone_to_root_transforms[bone_id] = inverse(mesh_data->bone_offset_transforms[bone_id]);
            }

            //TODO: since bone ids are now part of the vertex structure, they should be exported as part of vertex data from Blender
            //and read when vertex data is read
            for(uint32_t v = 0; v < mesh_data->vertex_data.size(); v++)
            {
                mesh_file.read(reinterpret_cast<char*>(&mesh_data->vertex_data[v].bone_id), sizeof(uint8_t));
                mesh_data->vertex_data[v].bone_id += mesh_data->bone_offset;
            }

            uint8_t pose_count = 0;
            mesh_file.read(reinterpret_cast<char*>(&pose_count), sizeof(uint8_t));

            for(uint8_t pose = 0; pose < pose_count; pose++)
            {
                uint8_t name_length;
                mesh_file.read(reinterpret_cast<char*>(&name_length), sizeof(uint8_t));
                std::string name(name_length, '\0');
                mesh_file.read(reinterpret_cast<char*>(name.data()), name_length);

                mesh_data->poses[name.c_str()].resize(mesh_data->bone_count);
                mesh_file.read(reinterpret_cast<char*>(mesh_data->poses[name.c_str()].data()), mesh_data->bone_count * sizeof(KeyFrame));
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
                mesh_data->animations[name.c_str()].key_frame_times.resize(keyframe_count);
                mesh_file.read(reinterpret_cast<char*>(mesh_data->animations[name.c_str()].key_frame_times.data()), keyframe_count * sizeof(float));

                mesh_data->animations[name.c_str()].key_frames.resize(mesh_data->bone_count);

                for(auto& key_frames : mesh_data->animations[name.c_str()].key_frames)
                {
                    key_frames.resize(keyframe_count);
                    mesh_file.read(reinterpret_cast<char*>(key_frames.data()), keyframe_count * sizeof(KeyFrame));
                }
            }
        }

        engine3d.updateVertexData(mesh_data->vb_alloc.vb, mesh_data->vb_alloc.data_offset, sizeof(VertexDefault) * vertex_count, mesh_data->vertex_data.data());

        m_mesh_data = std::shared_ptr<MeshData>(mesh_data);
        m_mesh_datas.emplace(filename, m_mesh_data);
    }

    m_src_key_frames.resize(m_mesh_data->bone_count, nullptr);
    m_dst_key_frames.resize(m_mesh_data->bone_count, nullptr);
    m_curr_pose.resize(m_mesh_data->bone_count);
    m_bone_transforms.resize(m_mesh_data->bone_count);
    resetPose();
    engine3d.updateBoneTransformData(m_mesh_data->bone_offset, m_mesh_data->bone_count, m_bone_transforms.data());
}

uint32_t Mesh::textureId() const
{
    return m_mesh_data->tex_id;
}

uint32_t Mesh::normalMapId() const
{
    return m_mesh_data->normal_map_id;
}

const Sphere& Mesh::boundingSphere() const
{
    return m_mesh_data->bounding_sphere;
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

const std::vector<AABB>& Mesh::aabbs() const
{
    return m_mesh_data->aabbs;
}

const std::vector<BoundingBox>& Mesh::bbs() const
{
    return m_mesh_data->bbs;
}

const std::vector<Sphere>& Mesh::spheres() const
{
    return m_mesh_data->spheres;
}

void Mesh::resetPose()
{
    if(m_mesh_data->poses.empty())
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

void Mesh::stopAnimation()
{
    animateToPose("pose_default");
}

void Mesh::animateToPose(std::string_view pose_name)
{
    m_play_animation = true;
    m_loop_animation = false;
    m_continue_animation = false;

    m_interrupt_pose = m_curr_pose;

    for(uint32_t bone_id = 0; bone_id < m_mesh_data->bone_count; bone_id++)
    {
        m_src_key_frames[bone_id] = &m_interrupt_pose[bone_id];
        m_dst_key_frames[bone_id] = &m_mesh_data->poses.at(pose_name.data())[bone_id];
    }

    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;
}

void Mesh::setAnimKeyframe(uint8_t keyframe)
{
    m_curr_anim_keyframe = keyframe;
    for(uint8_t i = 0; i < m_mesh_data->bone_count; i++)
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

void Mesh::playAnimation(std::string_view name)
{
    m_curr_anim = &m_mesh_data->animations.at(name.data());
    m_curr_anim_keyframe = -1;
    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;

    m_interrupt_pose = m_curr_pose;

    for(uint8_t i = 0; i < m_mesh_data->bone_count; i++)
    {
        m_src_key_frames[i] = &m_interrupt_pose[i];
        m_dst_key_frames[i] = &m_curr_anim->key_frames[i][0];
    }

    m_play_animation = true;
    m_continue_animation = true;
    m_loop_animation = true;
}

void Mesh::setPose(std::string_view pose_name)
{
    const Pose& pose = m_mesh_data->poses.at(pose_name.data());

    for(uint32_t bone_id = 0; bone_id < m_mesh_data->bone_count; bone_id++)
    {
        const auto q = pose[bone_id].rot;
        const auto trans = glm::translate(pose[bone_id].pos);

        const auto rot = mat4_cast(q);
        auto T = m_mesh_data->bone_to_root_transforms[bone_id] * trans * rot * m_mesh_data->bone_offset_transforms[bone_id];

        m_bone_transforms[bone_id] = T;

        if(m_mesh_data->bone_parent_ids[bone_id] != -1)
        {
            m_bone_transforms[bone_id] = m_bone_transforms[m_mesh_data->bone_parent_ids[bone_id]] * m_bone_transforms[bone_id];
        }
    }

    m_play_animation = false;
    m_curr_pose = m_mesh_data->poses.at(pose_name.data());
}

void Mesh::animationUpdate(Engine3D& engine3d, float dt)
{
    if(m_play_animation)
    {
        const float t = m_key_frame_elapsed_time / m_key_frame_time;

        for(uint32_t bone_id = 0; bone_id < m_mesh_data->bone_count; bone_id++)
        {
            const KeyFrame* src_key_frame = m_src_key_frames[bone_id];
            const KeyFrame* dst_key_frame = m_dst_key_frames[bone_id];

            m_curr_pose[bone_id].rot = glm::slerp(src_key_frame->rot, dst_key_frame->rot, t);
            m_curr_pose[bone_id].pos  = glm::mix(src_key_frame->pos, dst_key_frame->pos, t);

            const auto rot = mat4_cast(m_curr_pose[bone_id].rot);
            const auto trans = glm::translate(m_curr_pose[bone_id].pos);

            auto T = m_mesh_data->bone_to_root_transforms[bone_id] * trans * rot * m_mesh_data->bone_offset_transforms[bone_id];

            m_bone_transforms[bone_id] = T;

            if(m_mesh_data->bone_parent_ids[bone_id] != -1)
            {
                m_bone_transforms[bone_id] = m_bone_transforms[static_cast<uint8_t>(m_mesh_data->bone_parent_ids[bone_id])] * m_bone_transforms[bone_id];
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
    engine3d.updateBoneTransformData(m_mesh_data->bone_offset, m_mesh_data->bone_count, m_bone_transforms.data());
}

bool Mesh::forPlayer1(std::string_view anim_name)
{
    return !m_play_animation || (m_curr_anim != &m_mesh_data->animations.at(anim_name.data())) || !m_continue_animation;
}

bool Mesh::forPlayer2(std::string_view anim_name)
{
    return m_curr_anim == &m_mesh_data->animations.at(anim_name.data());
}

#if EDITOR_ENABLE
bool Mesh::rayIntersetion(const Ray& rayL, float min_d, float& d) const
{
    if(intersect(m_mesh_data->bounding_sphere, rayL, d))
    {
        //early out
        if(d >= min_d)
        {
            return false;
        }

        d = min_d;
        bool intersects = false;

        for(uint32_t v = 0; v < m_mesh_data->vertex_data.size(); v += 3)
        {
            //TODO: have a separate class for animated meshes or something, do this cleaner
            mat4x4 T0, T1, T2;
            if(m_bone_transforms.empty())
            {
                T0 = glm::identity<mat4x4>();
                T1 = glm::identity<mat4x4>();
                T2 = glm::identity<mat4x4>();
            }
            else
            {
                T0 = m_bone_transforms[m_mesh_data->vertex_data[v].bone_id - m_mesh_data->bone_offset];
                T1 = m_bone_transforms[m_mesh_data->vertex_data[v + 1].bone_id - m_mesh_data->bone_offset];
                T2 = m_bone_transforms[m_mesh_data->vertex_data[v + 2].bone_id - m_mesh_data->bone_offset];
            }

            const vec3 p0 = T0 * vec4(m_mesh_data->vertex_data[v].pos, 1.0f);
            const vec3 p1 = T1 * vec4(m_mesh_data->vertex_data[v + 1].pos, 1.0f);
            const vec3 p2 = T2 * vec4(m_mesh_data->vertex_data[v + 2].pos, 1.0f);

            float tri_d;

            if(intersect(rayL, p0, p1, p2, tri_d))
            {
                if(tri_d < d)
                {
                    d = tri_d;
                    intersects = true;
                }
            }
        }

        return intersects;
    }

    return false;
}
#endif
