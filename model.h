#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <unordered_map>
#include "mesh.h"

struct KeyFrame
{
    quat rot;
    vec3 pos;
};

struct Animation
{
    std::vector<std::vector<KeyFrame>> key_frames;
    std::vector<float> key_frame_times;
};

using Pose = std::vector<KeyFrame>;

class Model
{
    struct ModelData
    {
        std::vector<Mesh> meshes;

        Sphere bounding_sphere;

        //collision
        std::vector<AABB> aabbs;
        std::vector<BoundingBox> bbs;
        std::vector<Sphere> spheres;

        //animation
        uint8_t bone_count = 0;
        uint32_t bone_offset = 0;
        std::vector<int8_t> bone_parent_ids;
        std::vector<mat4x4> bone_offset_transforms;
        std::vector<mat4x4> bone_to_root_transforms;
        std::unordered_map<std::string, Pose> poses;
        std::unordered_map<std::string, Animation> animations;
    };

    static inline std::unordered_map<std::string, std::shared_ptr<const ModelData>> m_model_datas;

public:
    Model(Engine3D& engine3d, std::string_view filename);

    const std::vector<Mesh>& mehes() const;

    const Sphere& boundingSphere() const;

    const std::vector<AABB>& aabbs() const;
    const std::vector<BoundingBox>& bbs() const;
    const std::vector<Sphere>& spheres() const;

    void playAnimation(std::string_view);
    void stopAnimation();
    void setPose(std::string_view);
    void animationUpdate(Engine3D& engine3d, float dt);

    //TODO: these two
    bool forPlayer1(std::string_view);
    bool forPlayer2(std::string_view);

#if EDITOR_ENABLE
    bool rayIntersetion(const Ray& rayL, float min_d, float& d) const;
#endif

private:
    void resetPose();
    void animateToPose(std::string_view pose_name);
    void setAnimKeyframe(uint8_t keyframe);

    std::shared_ptr<const ModelData> m_model_data;

    //animation
    const Animation* m_curr_anim = nullptr;
    int m_curr_anim_keyframe = 0;
    float m_key_frame_time = 0.0f;
    float m_key_frame_elapsed_time = 0.0f;
    std::vector<const KeyFrame*> m_src_key_frames;
    std::vector<const KeyFrame*> m_dst_key_frames;
    bool m_loop_animation = false;
    bool m_play_animation = false;
    bool m_continue_animation = false;

    Pose m_curr_pose;
    Pose m_interrupt_pose;

    std::vector<mat4x4> m_bone_transforms;
};

#endif // MODEL_H
