#include "model.h"

Model::Model(Engine3D& engine3d, std::string_view filename)
{
    if(m_model_datas.contains(filename.data()))
    {
        m_model_data = m_model_datas[filename.data()];
    }
    else
    {
        ModelData* model_data = new ModelData;

        std::ifstream model_file(filename.data(), std::ios::binary);

        if(!model_file)
        {
            throw std::runtime_error(std::string("Failed to open model file: ") + filename.data());
        }

        model_file.read(reinterpret_cast<char*>(&model_data->bone_count), sizeof(uint8_t));

        if(model_data->bone_count != 0)
        {
            model_data->bone_offset = engine3d.requestBoneTransformBufferAllocation(model_data->bone_count);

            model_data->bone_parent_ids.resize(model_data->bone_count);
            model_file.read(reinterpret_cast<char*>(model_data->bone_parent_ids.data()), model_data->bone_count * sizeof(int8_t));

            model_data->bone_offset_transforms.resize(model_data->bone_count);
            model_file.read(reinterpret_cast<char*>(model_data->bone_offset_transforms.data()), model_data->bone_count * sizeof(mat4x4));

            model_data->bone_to_root_transforms.resize(model_data->bone_count);
            for(uint32_t bone_id = 0; bone_id < model_data->bone_count; bone_id++)
            {
                model_data->bone_to_root_transforms[bone_id] = inverse(model_data->bone_offset_transforms[bone_id]);
            }

            uint8_t pose_count = 0;
            model_file.read(reinterpret_cast<char*>(&pose_count), sizeof(uint8_t));

            for(uint8_t pose = 0; pose < pose_count; pose++)
            {
                uint8_t name_length;
                model_file.read(reinterpret_cast<char*>(&name_length), sizeof(uint8_t));
                std::string name(name_length, '\0');
                model_file.read(reinterpret_cast<char*>(name.data()), name_length);

                model_data->poses[name.c_str()].resize(model_data->bone_count);
                model_file.read(reinterpret_cast<char*>(model_data->poses[name.c_str()].data()), model_data->bone_count * sizeof(KeyFrame));
            }

            uint8_t animation_count = 0;
            model_file.read(reinterpret_cast<char*>(&animation_count), sizeof(uint8_t));

            for(uint8_t anim = 0; anim < animation_count; anim++)
            {
                uint8_t name_length;
                model_file.read(reinterpret_cast<char*>(&name_length), sizeof(uint8_t));
                std::string name(name_length, '\0');
                model_file.read(reinterpret_cast<char*>(name.data()), name_length);

                uint8_t keyframe_count = 0;
                model_file.read(reinterpret_cast<char*>(&keyframe_count), sizeof(uint8_t));
                model_data->animations[name.c_str()].key_frame_times.resize(keyframe_count);
                model_file.read(reinterpret_cast<char*>(model_data->animations[name.c_str()].key_frame_times.data()), keyframe_count * sizeof(float));

                model_data->animations[name.c_str()].key_frames.resize(model_data->bone_count);

                for(auto& key_frames : model_data->animations[name.c_str()].key_frames)
                {
                    key_frames.resize(keyframe_count);
                    model_file.read(reinterpret_cast<char*>(key_frames.data()), keyframe_count * sizeof(KeyFrame));
                }
            }
        }


        uint64_t mesh_count = 0;
        model_file.read(reinterpret_cast<char*>(&mesh_count), sizeof(uint64_t));

        model_data->meshes.reserve(mesh_count);

        for(uint64_t mesh_id = 0; mesh_id < mesh_count; mesh_id++)
        {
            model_data->meshes.emplace_back(engine3d, model_file, model_data->bone_offset);
        }

        model_file.read(reinterpret_cast<char*>(&model_data->bounding_sphere), sizeof(Sphere));

        uint8_t aabb_count = 0;
        model_file.read(reinterpret_cast<char*>(&aabb_count), sizeof(uint8_t));
        model_data->aabbs.resize(aabb_count);
        model_file.read(reinterpret_cast<char*>(model_data->aabbs.data()), aabb_count * sizeof(AABB));

        uint8_t bb_count = 0;
        model_file.read(reinterpret_cast<char*>(&bb_count), sizeof(uint8_t));
        model_data->bbs.resize(bb_count);
        model_file.read(reinterpret_cast<char*>(model_data->bbs.data()), bb_count * sizeof(BoundingBox));

        uint8_t sphere_count = 0;
        model_file.read(reinterpret_cast<char*>(&sphere_count), sizeof(uint8_t));
        model_data->spheres.resize(sphere_count);
        model_file.read(reinterpret_cast<char*>(model_data->spheres.data()), sphere_count * sizeof(Sphere));

        m_model_data = std::shared_ptr<ModelData>(model_data);
        m_model_datas.emplace(filename, m_model_data);
    }

    m_src_key_frames.resize(m_model_data->bone_count, nullptr);
    m_dst_key_frames.resize(m_model_data->bone_count, nullptr);
    m_curr_pose.resize(m_model_data->bone_count);
    m_bone_transforms.resize(m_model_data->bone_count);
    resetPose();
    engine3d.updateBoneTransformData(m_model_data->bone_offset, m_model_data->bone_count, m_bone_transforms.data());
}

const std::vector<Mesh>& Model::mehes() const
{
    return m_model_data->meshes;
}

const Sphere& Model::boundingSphere() const
{
    return m_model_data->bounding_sphere;
}

const std::vector<AABB>& Model::aabbs() const
{
    return m_model_data->aabbs;
}

const std::vector<BoundingBox>& Model::bbs() const
{
    return m_model_data->bbs;
}

const std::vector<Sphere>& Model::spheres() const
{
    return m_model_data->spheres;
}


void Model::resetPose()
{
    if(m_model_data->poses.empty())
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

void Model::stopAnimation()
{
    animateToPose("pose_default");
}

void Model::animateToPose(std::string_view pose_name)
{
    m_play_animation = true;
    m_loop_animation = false;
    m_continue_animation = false;

    m_interrupt_pose = m_curr_pose;

    for(uint32_t bone_id = 0; bone_id < m_model_data->bone_count; bone_id++)
    {
        m_src_key_frames[bone_id] = &m_interrupt_pose[bone_id];
        m_dst_key_frames[bone_id] = &m_model_data->poses.at(pose_name.data())[bone_id];
    }

    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;
}

void Model::setAnimKeyframe(uint8_t keyframe)
{
    m_curr_anim_keyframe = keyframe;
    for(uint8_t i = 0; i < m_model_data->bone_count; i++)
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

void Model::playAnimation(std::string_view name)
{
    m_curr_anim = &m_model_data->animations.at(name.data());
    m_curr_anim_keyframe = -1;
    m_key_frame_time = 0.2f;
    m_key_frame_elapsed_time = 0.0f;

    m_interrupt_pose = m_curr_pose;

    for(uint8_t i = 0; i < m_model_data->bone_count; i++)
    {
        m_src_key_frames[i] = &m_interrupt_pose[i];
        m_dst_key_frames[i] = &m_curr_anim->key_frames[i][0];
    }

    m_play_animation = true;
    m_continue_animation = true;
    m_loop_animation = true;
}

void Model::setPose(std::string_view pose_name)
{
    const Pose& pose = m_model_data->poses.at(pose_name.data());

    for(uint32_t bone_id = 0; bone_id < m_model_data->bone_count; bone_id++)
    {
        const auto q = pose[bone_id].rot;
        const auto trans = glm::translate(pose[bone_id].pos);

        const auto rot = mat4_cast(q);
        auto T = m_model_data->bone_to_root_transforms[bone_id] * trans * rot * m_model_data->bone_offset_transforms[bone_id];

        m_bone_transforms[bone_id] = T;

        if(m_model_data->bone_parent_ids[bone_id] != -1)
        {
            m_bone_transforms[bone_id] = m_bone_transforms[m_model_data->bone_parent_ids[bone_id]] * m_bone_transforms[bone_id];
        }
    }

    m_play_animation = false;
    m_curr_pose = m_model_data->poses.at(pose_name.data());
}

void Model::animationUpdate(Engine3D& engine3d, float dt)
{
    if(m_play_animation)
    {
        const float t = m_key_frame_elapsed_time / m_key_frame_time;

        for(uint32_t bone_id = 0; bone_id < m_model_data->bone_count; bone_id++)
        {
            const KeyFrame* src_key_frame = m_src_key_frames[bone_id];
            const KeyFrame* dst_key_frame = m_dst_key_frames[bone_id];

            m_curr_pose[bone_id].rot = glm::slerp(src_key_frame->rot, dst_key_frame->rot, t);
            m_curr_pose[bone_id].pos  = glm::mix(src_key_frame->pos, dst_key_frame->pos, t);

            const auto rot = mat4_cast(m_curr_pose[bone_id].rot);
            const auto trans = glm::translate(m_curr_pose[bone_id].pos);

            auto T = m_model_data->bone_to_root_transforms[bone_id] * trans * rot * m_model_data->bone_offset_transforms[bone_id];

            m_bone_transforms[bone_id] = T;

            if(m_model_data->bone_parent_ids[bone_id] != -1)
            {
                m_bone_transforms[bone_id] = m_bone_transforms[static_cast<uint8_t>(m_model_data->bone_parent_ids[bone_id])] * m_bone_transforms[bone_id];
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
    engine3d.updateBoneTransformData(m_model_data->bone_offset, m_model_data->bone_count, m_bone_transforms.data());
}

bool Model::forPlayer1(std::string_view anim_name)
{
    return !m_play_animation || (m_curr_anim != &m_model_data->animations.at(anim_name.data())) || !m_continue_animation;
}

bool Model::forPlayer2(std::string_view anim_name)
{
    return m_curr_anim == &m_model_data->animations.at(anim_name.data());
}

#if EDITOR_ENABLE
bool Model::rayIntersetion(const Ray& rayL, float min_d, float& d) const
{
    if(intersect(m_model_data->bounding_sphere, rayL, d))
    {
        //early out
        if(d >= min_d)
        {
            return false;
        }

        d = min_d;
        bool intersects = false;

        for(const auto& mesh : m_model_data->meshes)
        {
            for(uint32_t v = 0; v < mesh.vertexData().size(); v += 3)
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
                    T0 = m_bone_transforms[mesh.vertexData()[v].bone_id - m_model_data->bone_offset];
                    T1 = m_bone_transforms[mesh.vertexData()[v + 1].bone_id - m_model_data->bone_offset];
                    T2 = m_bone_transforms[mesh.vertexData()[v + 2].bone_id - m_model_data->bone_offset];
                }

                const vec3 p0 = T0 * vec4(mesh.vertexData()[v].pos, 1.0f);
                const vec3 p1 = T1 * vec4(mesh.vertexData()[v + 1].pos, 1.0f);
                const vec3 p2 = T2 * vec4(mesh.vertexData()[v + 2].pos, 1.0f);

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
        }

        return intersects;
    }

    return false;
}
#endif
