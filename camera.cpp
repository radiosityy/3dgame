#include "camera.h"

Camera::Camera(float ratio, float n, float f, float hfov)
    : m_aspect_ratio(ratio)
    , m_near(n)
    , m_far(f)
    , m_hfov(hfov)
    , m_vfov(2.0f * atanf(tanf(m_hfov / 2.0f) / m_aspect_ratio))
    , m_img_plane_distance(1.0f / std::tan(m_vfov / 2.0f))
{}

void Camera::walk(float d) noexcept
{
    m_pos = m_pos + d*m_forward;

    m_dirty_view = true;
}

void Camera::strafe(float d) noexcept
{
    m_pos = m_pos + d*m_right;

    m_dirty_view = true;
}

void Camera::tilt(float d) noexcept
{
    m_pos = m_pos + d*m_up;

    m_dirty_view = true;
}

void Camera::setPos(vec3 pos) noexcept
{
    m_pos = pos;

    m_dirty_view = true;
}

void Camera::setBasis(vec3 forward, vec3 up, vec3 right) noexcept
{
    m_forward = forward;
    m_up = up;
    m_right = right;
    m_dirty_view = true;
}

void Camera::setHFOV(float a) noexcept
{
    m_hfov = a;
    m_vfov = 2.0f * atanf(tanf(a / 2.0f) / m_aspect_ratio);
    m_img_plane_distance = 1.0f / std::tan(m_vfov / 2.0f);

    updateProj();
}

void Camera::setVFOV(float a) noexcept
{
    m_vfov = a;
    m_hfov = 2.0f * atanf(m_aspect_ratio*tanf(m_vfov / 2.0f));
    m_img_plane_distance = 1.0f / std::tan(m_vfov / 2.0f);

    updateProj();
}

void Camera::setNear(float n) noexcept
{
    m_near = n;

    updateProj();
}

void Camera::setFar(float f) noexcept
{
    m_far = f;

    updateProj();
}

void Camera::setAspectRatio(float r) noexcept
{
    m_aspect_ratio = r;

    updateProj();
}

void Camera::rotate(float angle) noexcept
{
    m_right = glm::rotateY<float>(m_right, angle);
    m_forward = glm::rotateY<float>(m_forward, angle);
    m_up = glm::rotateY<float>(m_up, angle);

    m_dirty_view = true;
}

void Camera::pitch(float angle) noexcept
{
    m_forward = glm::rotate<float>(m_forward, angle, m_right);
    m_up = glm::rotate<float>(m_up, angle, m_right);

    m_dirty_view = true;
}

mat4x4 Camera::VP() const noexcept
{
    updateView();
    updateProj();

    return m_proj*m_view;
}

float Camera::hvof() const noexcept
{
    return m_hfov;
}

float Camera::vfov() const noexcept
{
    return m_vfov;
}

float Camera::aspectRatio() const noexcept
{
    return m_aspect_ratio;
}

float Camera::far() const noexcept
{
    return m_far;
}

float Camera::near() const noexcept
{
    return m_near;
}

float Camera::imagePlaneDistance() const noexcept
{
    return m_img_plane_distance;
}

std::array<vec3, 8> Camera::viewFrustumPointsW() const noexcept
{
    const float vtan = 1.0f / m_img_plane_distance;

    const float nh2 = m_near * vtan;
    const float nw2 = m_aspect_ratio * nh2;
    const float fh2 = m_far * vtan;
    const float fw2 = m_aspect_ratio * fh2;

    updateView();

    std::array<vec3, 8> points;
    points[0] = m_inv_view * vec4(-nw2, -nh2, m_near, 1.0f);
    points[1] = m_inv_view * vec4(-nw2, nh2, m_near, 1.0f);
    points[2] = m_inv_view * vec4(nw2, nh2, m_near, 1.0f);
    points[3] = m_inv_view * vec4(nw2, -nh2, m_near, 1.0f);

    points[4] = m_inv_view * vec4(-fw2, -fh2, m_far, 1.0f);
    points[5] = m_inv_view * vec4(-fw2, fh2, m_far, 1.0f);
    points[6] = m_inv_view * vec4(fw2, fh2, m_far, 1.0f);
    points[7] = m_inv_view * vec4(fw2, -fh2, m_far, 1.0f);

    return points;
}

std::array<vec4, 6> Camera::viewFrustumPlanesW() const noexcept
{
    std::array<vec4, 6> planes;
    const auto points = viewFrustumPointsW();

    //near plane
    vec3 N = normalize(cross(points[1] - points[2], points[3] - points[2]));
    planes[0] = vec4(N, -dot(N, points[2]));

    //far plane
    N = normalize(cross(points[4] - points[7], points[6] - points[7]));
    planes[1] = vec4(N, -dot(N, points[7]));

    //left plane
    N = normalize(cross(points[0] - points[4], points[5] - points[4]));
    planes[2] = vec4(N, -dot(N, points[4]));

    //right plane
    N = normalize(cross(points[7] - points[3], points[2] - points[3]));
    planes[3] = vec4(N, -dot(N, points[3]));

    //top plane
    N = normalize(cross(points[6] - points[2], points[1] - points[2]));
    planes[4] = vec4(N, -dot(N, points[2]));

    //bottom plane
    N = normalize(cross(points[0] - points[3], points[7] - points[3]));
    planes[5] = vec4(N, -dot(N, points[3]));

    return planes;
}

void Camera::updateView() const noexcept
{
    if(m_dirty_view)
    {
        m_inv_view[0] = vec4(m_right, 0.0f);
        m_inv_view[1] = vec4(m_up, 0.0f);
        m_inv_view[2] = vec4(m_forward, 0.0f);
        m_inv_view[3] = vec4(m_pos, 1.0f);

        m_view[0][0] = m_right.x;
        m_view[1][0] = m_right.y;
        m_view[2][0] = m_right.z;
        m_view[0][1] = m_up.x;
        m_view[1][1] = m_up.y;
        m_view[2][1] = m_up.z;
        m_view[0][2] = m_forward.x;
        m_view[1][2] = m_forward.y;
        m_view[2][2] = m_forward.z;
        m_view[3][0] = -dot(m_right, m_pos);
        m_view[3][1] = -dot(m_up, m_pos);
        m_view[3][2] = -dot(m_forward, m_pos);

        m_view = glm::inverse(m_inv_view);

        m_dirty_view = false;
    }
}

void Camera::updateProj() const noexcept
{
    m_proj = glm::perspectiveLH_ZO(m_vfov, m_aspect_ratio, m_near, m_far);
}

const mat4x4& Camera::V() const noexcept
{
    updateView();
    return m_view;
}

const mat4x4& Camera::invV() const noexcept
{
    updateView();
    return m_inv_view;
}

const mat4x4& Camera::P() noexcept
{
    updateProj();
    return m_proj;
}

const vec3& Camera::pos() const noexcept
{
    return m_pos;
}

const vec3& Camera::forward() const noexcept
{
    return m_forward;
}

const vec3& Camera::up() const noexcept
{
    return m_up;
}

const vec3& Camera::right() const noexcept
{
    return m_right;
}

vec3 Camera::cursorProjW(const vec2& cur_pos_ndc) noexcept
{
    return invV() * vec4(cur_pos_ndc.x * m_aspect_ratio, cur_pos_ndc.y, imagePlaneDistance(), 0);
}
