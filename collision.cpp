#include "collision.h"
#include <stdexcept>
#include <vector>
#include <algorithm>

/*--- Helper Functions ---*/

std::pair<float, float> computeInterval(const std::array<vec3, 8> verts, vec3 axis)
{
    float min = dot(axis, verts[0]);
    float max = min;

    for(uint32_t i = 1; i < verts.size(); i++)
    {
        float value = dot(axis, verts[i]);
        if(value < min)
        {
            min = value;
        }
        else if(value > max)
        {
            max = value;
        }
    }

    return {min, max};
}

/*--- Intersection tests ---*/

bool intersect(const Plane& plane0, const Plane& plane1)
{
    throw std::runtime_error("Not implemented!");
}

bool intersect(const Plane& plane, const AABB& aabb)
{
    throw std::runtime_error("Not implemented!");
}

bool intersect(const AABB& aabb, const Plane& plane)
{
    return intersect(plane, aabb);
}

bool intersect(const Plane& plane, const BoundingBox& bb)
{
    throw std::runtime_error("Not implemented!");
}

bool intersect(const BoundingBox& bb, const Plane& plane)
{
    return intersect(plane, bb);
}

bool intersect(const Plane& plane, const Sphere& sphere)
{
    return std::abs(dot(sphere.m_center, plane.m_normal) - plane.m_offset) <= sphere.m_radius;
}

bool intersect(const Sphere& sphere, const Plane& plane)
{
    return intersect(plane, sphere);
}

bool intersect(const AABB& aabb0, const AABB& aabb1)
{
    return
            (aabb0.m_max.x >= aabb1.m_min.x) && (aabb0.m_min.x < aabb1.m_max.x) &&
            (aabb0.m_max.y >= aabb1.m_min.y) && (aabb0.m_min.y < aabb1.m_max.y) &&
            (aabb0.m_max.z >= aabb1.m_min.z) && (aabb0.m_min.z < aabb1.m_max.z);
}

bool intersect(const AABB& aabb, const BoundingBox& bb)
{
    const static std::vector<vec3> aabb_face_normals =
    {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    for(const auto& axis : bb.m_face_normals)
    {
        const auto [min0, max0] = bb.computeInterval(axis);
        const auto [min1, max1] = aabb.computeInterval(axis);

        if((max1 < min0) || (max0 < min1))
        {
            return false;
        }
    }

    for(const auto& axis : aabb_face_normals)
    {
        const auto [min0, max0] = bb.computeInterval(axis);
        const auto [min1, max1] = aabb.computeInterval(axis);

        if((max1 < min0) || (max0 < min1))
        {
            return false;
        }
    }

    for(const auto& axis0 : bb.m_face_normals)
    {
        for(const auto& axis1 : aabb_face_normals)
        {
            const auto axis = cross(axis0, axis1);

            const auto [min0, max0] = bb.computeInterval(axis);
            const auto [min1, max1] = aabb.computeInterval(axis);

            if((max1 < min0) || (max0 < min1))
            {
                return false;
            }
        }
    }

    return true;
}

bool intersect(const BoundingBox& bb, const AABB& aabb)
{
    return intersect(aabb, bb);
}

bool intersect(const AABB& aabb, const Sphere& s)
{
    float dmin = 0;
    for(uint32_t i = 0; i < 3; i++)
    {
        if(s.m_center[i] < aabb.m_min[i])
        {
            dmin += std::pow(s.m_center[i] - aabb.m_min[i], 2);
        }
        else if(s.m_center[i] > aabb.m_max[i])
        {
            dmin += std::pow(s.m_center[i] - aabb.m_max[i], 2);
        }
    }

    return dmin <= (s.m_radius * s.m_radius);
}

bool intersect(const Sphere& s, const AABB& aabb)
{
    return intersect(aabb, s);
}

bool intersect(const BoundingBox& bb0, const BoundingBox& bb1)
{
    for(const auto& axis : bb0.m_face_normals)
    {
        const auto [min0, max0] = bb0.computeInterval(axis);
        const auto [min1, max1] = bb1.computeInterval(axis);

        if((max1 < min0) || (max0 < min1))
        {
            return false;
        }
    }

    for(const auto& axis : bb1.m_face_normals)
    {
        const auto [min0, max0] = bb0.computeInterval(axis);
        const auto [min1, max1] = bb1.computeInterval(axis);

        if((max1 < min0) || (max0 < min1))
        {
            return false;
        }
    }

    for(const auto& axis0 : bb0.m_face_normals)
    {
        for(const auto& axis1 : bb1.m_face_normals)
        {
            const auto axis = cross(axis0, axis1);

            const auto [min0, max0] = bb1.computeInterval(axis);
            const auto [min1, max1] = bb0.computeInterval(axis);

            if((max1 < min0) || (max0 < min1))
            {
                return false;
            }
        }
    }

    return true;
}

bool intersect(const BoundingBox& bb, const Sphere& sphere)
{
    vec3 res(0.0f, 0.0f, 0.0f);

    for(const auto& a : bb.m_face_normals)
    {
        auto axis = normalize(a);
        const auto [bb_min, bb_max] = bb.computeInterval(axis);
        const float s_center = dot(sphere.m_center, axis);

        res += axis * std::clamp(s_center, bb_min, bb_max);
    }

    return distance(res, sphere.m_center) <= sphere.m_radius;
}

bool intersect(const Sphere& sphere, const BoundingBox& bb)
{
    return intersect(bb, sphere);
}

bool intersect(const Sphere& s0, const Sphere& s1)
{
    return glm::distance(s0.m_center, s1.m_center) >= (s0.m_radius + s1.m_radius);
}

//AABB

AABB::AABB(vec3 p_min, vec3 p_max)
    : m_min(p_min)
    , m_max(p_max)
{
    m_verts[0] = p_min;
    m_verts[1] = vec3(p_min.x, p_min.y, p_max.z);
    m_verts[2] = vec3(p_min.x, p_max.y, p_max.z);
    m_verts[3] = vec3(p_min.x, p_min.y, p_min.z);
    m_verts[4] = vec3(p_max.x, p_min.y, p_min.z);
    m_verts[5] = vec3(p_max.x, p_min.y, p_max.z);
    m_verts[6] = p_max;
    m_verts[7] = vec3(p_max.x, p_max.y, p_min.z);
}

vec3 AABB::min() const
{
    return m_min;
}

vec3 AABB::max() const
{
    return m_max;
}

void AABB::transform(vec3 translation, vec3 scaling)
{
    m_min = m_min * scaling + translation;
    m_max = m_max * scaling + translation;

    for(auto& v : m_verts)
    {
        v = v * scaling + translation;
    }
}

std::pair<float, float> AABB::computeInterval(vec3 axis) const
{
    return ::computeInterval(m_verts, axis);
}

//Bounding Box
BoundingBox::BoundingBox(const std::array<vec3, 8>& verts, const std::array<vec3, 3>& face_normals)
    : m_verts(verts)
    , m_face_normals(face_normals)
{}

void BoundingBox::transform(vec3 translation, quat rotation, vec3 scaling)
{
    const auto rot = mat4_cast(rotation);
    const auto W = translate(translation) * rot * scale(scaling);
    const auto invW = inverse(W);

    for(auto& v : m_verts)
    {
        v = W * vec4(v, 1.0f);
    }

    for(auto& n : m_face_normals)
    {
        n = vec4(n, 0.0f) * invW;
    }
}

std::pair<float, float> BoundingBox::computeInterval(vec3 axis) const
{
    return ::computeInterval(m_verts, axis);
}

//Sphere
Sphere::Sphere(vec3 center, float radius)
    : m_center(center)
    , m_radius(radius)
{}

void Sphere::transform(vec3 translation, vec3 scaling)
{
    m_center += translation;
    m_radius *= std::max(std::max(scaling.x, scaling.y), scaling.z);
}

bool Sphere::intersect(const std::array<vec4, 6> frustum) const
{
    for(const auto& plane : frustum)
    {
        const float k = m_center.x * plane.x + m_center.y * plane.y + m_center.z * plane.z + plane.w;
        if(k < -m_radius)
        {
            return false;
        }
    }

    return true;
}

bool intersect(const Sphere& s, const Ray& ray, float& d)
{
    const vec3 v = ray.origin - s.m_center;

    const float a = dot(ray.dir, ray.dir);
    const float b = 2.0f * dot(v, ray.dir);
    const float c = dot(v, v) - s.m_radius * s.m_radius;

    const float delta = b*b - 4.0f*a*c;

    if(delta < 0)
    {
        return false;
    }

    if(delta == 0.0f)
    {
        d = -b / (2.0f * a);
        return true;
    }

    const float delta_sqrt = std::sqrt(delta);

    if(a < 0.0f)
    {
        d = (-b + delta_sqrt) / (2.0f * a);
    }
    else
    {
        d = (-b - delta_sqrt) / (2.0f * a);
    }

    return true;
}

bool intersect(const Ray& ray, const Sphere& s, float& d)
{
    return intersect(s, ray, d);
}

//ray-triangle
bool intersect(const Ray& ray, const vec3& p0, const vec3& p1, const vec3& p2, float& d)
{
    //TODO: check if ray needs to be normalized here
    const glm::vec3 e1 = p1 - p0;
    const glm::vec3 e2 = p2 - p0;
    const glm::vec3 m = ray.origin - p0;

    const glm::vec3 c1 = glm::cross(ray.dir, e2);
    const glm::vec3 c2 = glm::cross(m, e1);
    const float a = glm::dot(e1, c1);

    d = glm::dot(e2, c2) / a;
    const float u = glm::dot(m, c1) / a;
    const float v = glm::dot(ray.dir, c2) / a;

    return d > 0 && u >= 0 && v >= 0 && u+v <= 1;
}

//ray-aabb
bool intersect(const Ray& ray, const AABB& aabb)
{
    const vec3 t_min = (aabb.m_min - ray.origin) / ray.dir;
    const vec3 t_max = (aabb.m_max - ray.origin) / ray.dir;
    const vec3 t1 = min(t_min, t_max);
    const vec3 t2 = max(t_min, t_max);
    const float t_near = std::max(std::max(t1.x, t1.y), t1.z);
    const float t_far = std::min(std::min(t2.x, t2.y), t2.z);

    return t_far >= t_near;
}

#if 0
//triangle-aabb
static bool triangleAABBAxisTest(const vec3& axis, const vec3& aabb_extents, const vec3& t0, const vec3& t1, const vec3& t2)
{
    const float p0 = glm::dot(t0, axis);
    const float p1 = glm::dot(t1, axis);
    const float p2 = glm::dot(t2, axis);
    const float r = dot(aabb_extents, glm::abs(axis));

    return (-max(p0, p1, p2) <= r) && (min(p0, p1, p2) <= r);
}

bool intersect(const AABB& aabb, const vec3& p0, const vec3& p1, const vec3& p2)
{
    //TODO: put aabb center and extents as a field in the AABB struct and precalculate it only once (?)
    const vec3 aabb_center = (aabb.m_min + aabb.m_max) / 2.0f;
    const vec3 aabb_extents = aabb.m_max - aabb_center;

    const vec3 t0 = p0 - aabb_center;
    const vec3 t1 = p1 - aabb_center;
    const vec3 t2 = p2 - aabb_center;

    //triangle normals (3 axes)
    if(min(p0.x, p1.x, p2.x) > aabb.m_max.x) return false;
    if(max(p0.x, p1.x, p2.x) < aabb.m_min.x) return false;
    if(min(p0.y, p1.y, p2.y) > aabb.m_max.y) return false;
    if(max(p0.y, p1.y, p2.y) < aabb.m_min.y) return false;
    if(min(p0.z, p1.z, p2.z) > aabb.m_max.z) return false;
    if(max(p0.z, p1.z, p2.z) < aabb.m_min.z) return false;

    //aabb/triangle sides normal cross products (9 axes)
    const vec3 e0 = t1 - t0;
    const vec3 e1 = t2 - t1;
    const vec3 e2 = t0 - t2;

    //TODO: don't calculate cross products here as the normal vectors are constant and we can hardcode the results here
    if(!triangleAABBAxisTest(glm::cross(vec3(1.0f, 0.0f, 0.0f), e0), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(1.0f, 0.0f, 0.0f), e1), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(1.0f, 0.0f, 0.0f), e2), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 1.0f, 0.0f), e0), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 1.0f, 0.0f), e1), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 1.0f, 0.0f), e2), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 0.0f, 1.0f), e0), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 0.0f, 1.0f), e1), aabb_extents, t0, t1, t2)) return false;
    if(!triangleAABBAxisTest(glm::cross(vec3(0.0f, 0.0f, 1.0f), e2), aabb_extents, t0, t1, t2)) return false;

    //triangle plane normal (1 axis)
    //TODO: do we use the triangle points offset by aabb center or the original ones?
    //TODO: do we need to normalize this normal?
    const vec3 tn = glm::cross(e0, e1);
    const float r = glm::dot(aabb_extents, tn);
    const float d = -glm::dot(tn, t0);
    const float s = glm::dot(tn, aabb_center) - d;

    return std::abs(s) <= r;
}
#endif
