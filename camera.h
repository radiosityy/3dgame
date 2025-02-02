#ifndef CAMERA_H
#define CAMERA_H

#include "geometry.h"

#undef far
#undef near

class Camera
{
public:
    Camera(float ratio, float n, float f, float hfov);

    Camera (const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera (Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    void walk(float d) noexcept;
    void strafe(float d) noexcept;
    void pitch(float angle) noexcept;
    void rotate(float angle) noexcept;
    void tilt(float d) noexcept;

    void setPos(vec3) noexcept;
    void setBasis(vec3 forward, vec3 up, vec3 right) noexcept;
    void setHFOV(float) noexcept;
    void setVFOV(float) noexcept;
    void setNear(float) noexcept;
    void setFar(float) noexcept;
    void setAspectRatio(float) noexcept;

    float hvof() const noexcept;
    float vfov() const noexcept;
    float aspectRatio() const noexcept;
    float far() const noexcept;
    float near() const noexcept;
    float imagePlaneDistance() const noexcept;
    std::array<vec3, 8> viewFrustumPointsW() const noexcept;
    std::array<vec4, 6> viewFrustumPlanesW() const noexcept;

    mat4x4 VP() const noexcept;
    const mat4x4& V() const noexcept;
    const mat4x4& invV() const noexcept;
    const mat4x4& P() noexcept;

    const vec3& pos() const noexcept;
    const vec3& forward() const noexcept;
    const vec3& up() const noexcept;
    const vec3& right() const noexcept;

    vec3 cursorProjW(const vec2& cur_pos_ndc) noexcept;

private:
    void updateView() const noexcept;
    void updateProj() const noexcept;

    mutable bool m_dirty_view = true;

    mutable mat4x4 m_view = glm::identity<mat4x4>();
    mutable mat4x4 m_inv_view = glm::identity<mat4x4>();
    mutable mat4x4 m_proj = glm::identity<mat4x4>();

    vec3 m_pos = vec3(0.0f, 0.0f, 0.0f);
    vec3 m_forward = vec3(0.0f, 0.0f, 1.0f);
    vec3 m_right = vec3(1.0f, 0.0f, 0.0f);
    vec3 m_up = vec3(0.0f, 1.0f, 0.0f);

    float m_aspect_ratio = 1.0f;
    float m_near = 1.0f;
    float m_far = 10.0f;
    float m_hfov = 1.0f;
    float m_vfov = 1.0f;

    float m_img_plane_distance = 0.0f;
};

#endif //CAMERA_H
