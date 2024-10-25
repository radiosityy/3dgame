#ifndef CAMERA_H
#define CAMERA_H

#include "geometry.h"

#undef far
#undef near

class Camera
{
public:
    Camera(float ratio, float n, float f, float hfov);

    /*explicitly disable copy and move operations*/
    Camera (const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera (Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    /*moves camera forward and backward*/
    void walk(float d) noexcept;
    /*moves camera left and right*/
    void strafe(float d) noexcept;
    /*rotates camera up and down*/
    void pitch(float angle) noexcept;
    /*rotates camera left and right*/
    void rotate(float angle) noexcept;
    /*tilts camera up or down*/
    void tilt(float d) noexcept;

    /*sets camera position*/
    void setPos(vec3) noexcept;
    /*sets camera basis vectors*/
    void setBasis(vec3 forward, vec3 up, vec3 right) noexcept;
    /*set camera's horizontal FOV angle*/
    void setHFOV(float) noexcept;
    /*set camera's vertical FOV angle*/
    void setVFOV(float) noexcept;
    /*set near plane*/
    void setNear(float) noexcept;
    /*set far plane*/
    void setFar(float) noexcept;
    /*sets aspect ratio (width/height)*/
    void setAspectRatio(float) noexcept;

    float hvof() const noexcept;
    float vfov() const noexcept;
    float aspectRatio() const noexcept;
    float far() const noexcept;
    float near() const noexcept;
    float imagePlaneDistance() const noexcept;
    std::array<vec3, 8> viewFrustumPointsW() const noexcept;
    std::array<vec4, 6> viewFrustumPlanesW() const noexcept;

    /*updates and returns ViewProjection matrix*/
    mat4x4 VP() const noexcept;
    /*updates and returns View matrix*/
    const mat4x4& V() const noexcept;
    const mat4x4& invV() const noexcept;
    /*updates and returns Projection matrix*/
    const mat4x4& P() noexcept;

    const vec3& pos() const noexcept;
    const vec3& forward() const noexcept;
    const vec3& up() const noexcept;
    const vec3& right() const noexcept;

    vec3 cursorProjW(const vec2& cur_pos_ndc) noexcept;

private:
    void updateView() const noexcept;
    void updateProj() const noexcept;

    /*flag whether proj matrix needs to be updated*/
    mutable bool m_dirty_view;

    /*view and projection matrices*/
    mutable mat4x4 m_view;
    mutable mat4x4 m_inv_view;
    mutable mat4x4 m_proj;

    /*camera position*/
    vec3 m_pos;
    /*camera forward unit vector*/
    vec3 m_forward;
    /*camera up unit vector*/
    vec3 m_up;
    /*camera right unit vector*/
    vec3 m_right;

    /*near plane*/
    float m_near;
    /*far plane*/
    float m_far;
    /*FOV angles*/
    float m_vfov;
    float m_hfov;
    /*aspect ratio (width/height)*/
    float m_aspect_ratio;
};

#endif //CAMERA_H
