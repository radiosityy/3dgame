#ifndef OBJECT_H
#define OBJECT_H

#include "collision.h"
#include "render_data.h"
#include <vector>
#include "mesh.h"

class Object
{
public:
    Object(Renderer& renderer, std::ifstream& scene_file);
    Object(Renderer& renderer, std::string_view mesh_filename, RenderMode render_mode, vec3 pos = vec3(0.0f, 0.0f, 0.0f), vec3 scale = vec3(1.0f, 1.0f, 1.0f), quat rot = quat(1.0f, 0.0f, 0.0f, 0.0f));

    bool cull(const std::array<vec4, 6>& frustum_planes_W);

    void update(Renderer& renderer, float dt);
    void draw(Renderer& renderer);

    const std::vector<AABB>& aabbs() const;
    const std::vector<BoundingBox>& bbs() const;
    const std::vector<Sphere>& spheres() const;

    vec3 pos() const;

    vec3 velocity() const;

    void setRot(const mat4x4&);
    void setPos(vec3 pos);
    void move(vec3 d);

    void setVelocity(vec3 v);

    void setVisible(bool);
    bool isVisible() const;

    void playAnimation(std::string_view);
    void stopAnimation();
    void setPose(std::string_view);

#if EDITOR_ENABLE
    const quat& rotq() const;

    void setRotation(quat rot);
    void rotate(vec3 axis, float a);
    void rotateX(float a);
    void rotateY(float a);
    void rotateZ(float a);

    vec3 scale() const;
    void setScale(vec3);

    bool isSerializable() const;
    void setSerializable(bool);
    void serialize(std::ofstream& outfile) const;

    void drawHighlight(Renderer& renderer);

    bool rayIntersetion(const Ray& rayW, float min_d, float& d) const;
#endif

protected:
    std::unique_ptr<Mesh> m_mesh;

    vec3 m_pos = {0.0f, 0.0f, 0.0f};
    vec3 m_scale = {1.0f, 1.0f, 1.0f};
    mat4x4 m_rot = glm::identity<mat4x4>();
#if EDITOR_ENABLE
    quat m_rotq = quat(1.0f, 0.0f, 0.0f, 0.0f);
#endif

    vec3 m_velocity = {0.0f, 0.0f, 0.0f};

    RenderMode m_render_mode = RenderMode::Default;

private:
    std::vector<InstanceVertexData> m_instance_data;
    uint32_t m_instance_id = 0;
#if EDITOR_ENABLE
    std::string m_mesh_filename;
#endif
    bool m_visible = true;
    bool m_serialiable = true;
};

#endif // OBJECT_H
