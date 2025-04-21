#ifndef MESH_H
#define MESH_H

#include <string_view>
#include "vertex.h"
#include "collision.h"
#include "shaders/shader_constants.h"
#include "renderer.h"

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

struct MeshData
{
#if EDITOR_ENABLE
    //TODO: do we even need vertex data even in editor?
    std::vector<VertexDefault> vertex_data;
#endif
    VertexBufferAllocation vb_alloc;
    uint32_t tex_id = 0;
    uint32_t normal_map_id = NORMAL_MAP_ID_NONE;
};

class Mesh
{
public:
    Mesh(const MeshData* mesh_data) : m_mesh_data(mesh_data){};

    uint32_t textureId() const;
    uint32_t normalMapId() const;
    VertexBuffer* vertexBuffer() const;
    uint32_t vertexBufferOffset() const;
    uint32_t vertexCount() const;
    const std::vector<VertexDefault>& vertexData() const;
#if EDITOR_ENABLE
    bool rayIntersetion(const Ray& rayL, float min_d, float& d) const;
#endif

protected:
    const MeshData* const m_mesh_data = nullptr;
};

struct AnimatedMeshData
{
    uint8_t bone_count = 0;
    std::vector<int8_t> bone_parent_ids;
    std::vector<mat4x4> bone_offset_transforms;
    std::vector<mat4x4> bone_to_root_transforms;
    std::unordered_map<std::string, Pose> poses;
    std::unordered_map<std::string, Animation> animations;
};

class AnimatedMesh : public Mesh
{
public:
    AnimatedMesh(Renderer& renderer, const MeshData* mesh_data, const AnimatedMeshData* anim_mesh_data);

    uint32_t boneOffset() const;

    void playAnimation(std::string_view);
    void stopAnimation();
    void setPose(std::string_view);
    void animationUpdate(Renderer& renderer, float dt);
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

    const AnimatedMeshData* const m_animated_mesh_data = nullptr;

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

    uint32_t m_bone_offset = 0;
    std::vector<mat4x4> m_bone_transforms;
};

class MeshManager
{
public:
    void loadMeshData(Renderer& renderer, std::string_view mesh_filename);
    Mesh mesh(std::string_view mesh_name) const;
    AnimatedMesh animatedMesh(Renderer&, std::string_view mesh_name) const;
#if EDITOR_ENABLE
    uint64_t meshDataSize() const;
    const std::vector<std::string>& meshFilenames() const;
#endif

private:
    MeshData readMeshData(Renderer& renderer, std::ifstream& mesh_file);

    std::unordered_map<std::string, const MeshData> m_mesh_data;
    std::unordered_map<std::string, std::pair<const MeshData, const AnimatedMeshData>> m_animated_mesh_data;
#if EDITOR_ENABLE
    uint64_t m_mesh_data_size = 0;
    std::vector<std::string> m_mesh_filenames;
#endif
};

#endif //MESH_H
