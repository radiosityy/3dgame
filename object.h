#ifndef OBJECT_H
#define OBJECT_H

#include "collision.h"
#include "render_data.h"
#include <vector>
#include <map>
#include "model.h"

class Object
{
public:
    Object(Renderer& renderer, std::ifstream& scene_file);
    Object(Renderer& renderer, std::string_view model_filename, RenderMode render_mode, vec3 pos = vec3(0.0f, 0.0f, 0.0f), vec3 scale = vec3(1.0f, 1.0f, 1.0f), quat rot = quat(1.0f, 0.0f, 0.0f, 0.0f));

    bool cull(const std::array<vec4, 6>& frustum_planes_W);

    void update(Renderer& renderer, float dt);
    void draw(Renderer& renderer);

    const std::vector<AABB>& aabbs() const;
    const std::vector<BoundingBox>& bbs() const;
    const std::vector<Sphere>& spheres() const;

    vec3 pos() const;
    vec3 scale() const;
    const quat& rot() const;

    vec3 acceleration() const;
    vec3 velocity() const;
    float rotVelocity() const;

    void setPos(vec3 pos);
    void move(vec3 d);

    void setRotation(quat rot);
    void rotate(vec3 axis, float a);
    void rotateX(float a);
    void rotateY(float a);
    void rotateZ(float a);

    void setScale(vec3);

    void setVelocity(vec3 v);
    void setAcceleration(vec3 a);

    void setVisible(bool);
    bool isVisible() const;

    void playAnimation(std::string_view);
    void stopAnimation();
    void setPose(std::string_view);

#if EDITOR_ENABLE
    bool isSerializable() const;
    void setSerializable(bool);
    void serialize(std::ofstream& outfile) const;

    void drawHighlight(Renderer& renderer);

    bool rayIntersetion(const Ray& rayW, float min_d, float& d) const;
#endif

protected:
    std::unique_ptr<Model> m_model;

    vec3 m_pos = {0.0f, 0.0f, 0.0f};
    vec3 m_scale = {1.0f, 1.0f, 1.0f};
    quat m_rot = quat(1.0f, 0.0f, 0.0f, 0.0f);

    float m_rot_velocity = 0.0f;
    vec3 m_velocity = {0.0f, 0.0f, 0.0f};
    vec3 m_acc = {0.0f, 0.0f, 0.0f};

    RenderMode m_render_mode = RenderMode::Default;

private:
    std::vector<InstanceVertexData> m_instance_data;
    uint32_t m_instance_id = 0;
#if EDITOR_ENABLE
    std::string m_model_filename;
#endif
    bool m_visible = true;
    bool m_serialiable = true;
};

#endif // OBJECT_H
