#ifndef COLLISION_H
#define COLLISION_H

#include "geometry.h"

class Plane;
class AABB;
class BoundingBox;
class Sphere;

struct Ray
{
    Ray() = default;
    Ray(const vec3& _origin, const vec3 _dir)
        : origin(_origin)
        , dir(_dir)
    {}

    void transform(const mat4x4& T)
    {
        origin = T * vec4(origin, 1.0f);
        dir = T * vec4(dir, 0.0f);
    }

    Ray transformed(const mat4x4& T) const
    {
        Ray ray(T * vec4(origin, 1.0f), T * vec4(dir, 0.0f));
        return ray;
    }

    void normalize()
    {
        dir = glm::normalize(dir);
    }

    vec3 origin;
    vec3 dir;
};

class Plane
{
    friend bool intersect(const Plane&, const Plane&);
    friend bool intersect(const Plane&, const AABB&);
    friend bool intersect(const Plane&, const BoundingBox&);
    friend bool intersect(const Plane&, const Sphere&);

private:
    vec3 m_normal;
    float m_offset;
};

class AABB
{
    friend bool intersect(const Plane&, const AABB&);
    friend bool intersect(const AABB&, const AABB&);
    friend bool intersect(const AABB&, const BoundingBox&);
    friend bool intersect(const AABB&, const Sphere&);
    friend bool intersect(const Ray&, const AABB&);

public:
    AABB() = default;
    AABB(vec3 p_min, vec3 p_max);

    vec3 min() const;
    vec3 max() const;

    void transform(vec3 translation, vec3 scaling);
    std::pair<float, float> computeInterval(vec3 axis) const;

private:
    vec3 m_min;
    vec3 m_max;
    std::array<vec3, 8> m_verts;
};

class BoundingBox
{
    friend bool intersect(const Plane&, const BoundingBox&);
    friend bool intersect(const AABB&, const BoundingBox&);
    friend bool intersect(const BoundingBox&, const BoundingBox&);
    friend bool intersect(const BoundingBox&, const Sphere&);

public:
    BoundingBox() = default;
    BoundingBox(const std::array<vec3, 8>& verts, const std::array<vec3, 3>& face_normals);

    void transform(vec3 translation, quat rotation, vec3 scaling);
    std::pair<float, float> computeInterval(vec3 axis) const;

private:
    std::array<vec3, 8> m_verts;
    std::array<vec3, 3> m_face_normals;
};

class Sphere
{
    friend bool intersect(const Plane&, const Sphere&);
    friend bool intersect(const AABB&, const Sphere&);
    friend bool intersect(const BoundingBox&, const Sphere&);
    friend bool intersect(const Sphere&, const Sphere&);
    friend bool intersect(const Sphere&, const Ray& ray, float& d);
    friend bool intersect(const Ray& ray, const Sphere&, float& d);

public:
    Sphere() = default;
    Sphere(vec3 center, float radius);

    void transform(vec3 translation, vec3 scaling);

    bool intersect(const std::array<vec4, 6> frustum) const;

private:
    vec3 m_center;
    float m_radius;
};

bool intersect(const BoundingBox& bb, const AABB& aabb);
bool intersect(const Sphere& sphere, const AABB& aabb);
bool intersect(const Sphere& sphere, const BoundingBox& bb);
bool intersect(const Ray& ray, const vec3& p0, const vec3& p1, const vec3& p2, float& d);

#endif // COLLISION_H
