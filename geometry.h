#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <array>
#include <cmath>
#include <cstdint>
#include <concepts>
#include <numbers>

inline constexpr float pi = std::numbers::pi_v<float>;
inline constexpr float pi_2 = std::numbers::pi_v<float> / 2.0f;

constexpr inline float radToDeg(float rad)
{
    return rad * (360.0f / (2.0f * pi));
}

constexpr inline float degToRad(float deg)
{
    return deg * ((2.0f * pi) / 360.0f);
}

template<std::unsigned_integral T>
constexpr inline T roundUp(T value, T up)
{
    return ((value - static_cast<T>(1)) / up + static_cast<T>(1)) * up;
}

template<std::unsigned_integral T>
constexpr inline T roundDown(T value, T down)
{
    return value / down * down;
}

template<std::unsigned_integral T>
constexpr inline T divideRoundUp(T value, T divisor)
{
    return (value - static_cast<T>(1)) / divisor + static_cast<T>(1);
}

#if 0
struct LineSegment
{
    bool intersects(const LineSegment& rhs) const noexcept;
    static bool intersects(const LineSegment& lhs, const LineSegment& rhs) noexcept;

    Point2D p0;
    Point2D p1;
};
#endif

//TODO: if Quad is only used for scissors then it should store ints/uints, not floats
struct Quad
{
    static constexpr Quad defaultScissor()
    {
        return Quad{0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    }

    float x;
    float y;
    float width;
    float height;
};

constexpr inline Quad quadOverlap(const Quad& q1, const Quad& q2)
{
    const float min_x = std::max(q1.x, q2.x);
    const float max_x = std::min(q1.x + q1.width, q2.x + q2.width);
    const float min_y = std::max(q1.y, q2.y);
    const float max_y = std::min(q1.y + q1.height, q2.y + q2.height);

    return {min_x, min_y, max_x - min_x, max_y - min_y};
}

template<class T>
auto max(T t0, T t1)
{
    return std::max<T>(t0, t1);
}

template<class T, class... Args>
auto max(T t0, T t1, Args... args)
{
    return max(max(t0, t1), args...);
}

template<class T>
auto min(T t0, T t1)
{
    return std::min<T>(t0, t1);
}

template<class T, class... Args>
auto min(T t0, T t1, Args... args)
{
    return min(min(t0, t1), args...);
}

#define USE_GLM 1

#if USE_GLM

#define GLM_FORCE_INLINE
#define GLM_FORCE_SSE4
#define GLM_FORCE_QUAT_DATA_WXYZ

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/quaternion_geometric.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using quat = glm::quat;
using mat4x4 = glm::mat4x4;
using mat3x3 = glm::mat3x3;

#else
template<class T>
class vector2;

template<class T>
class vector3;

template<class T>
class vector4;

/*------------------------------------------- mat3x3 ---------------------------------------------*/

template <class T>
class matrix3x3
{
    template<class V>
    friend constexpr vector3<V> operator*(const vector3<V>& v, const matrix3x3<V>& m);

public:
    constexpr matrix3x3<T>()
        : m_11(1)
        , m_12(0)
        , m_13(0)
        , m_21(0)
        , m_22(1)
        , m_23(0)
        , m_31(0)
        , m_32(0)
        , m_33(1)
    {}

    static constexpr matrix3x3 identity() noexcept
    {
        return matrix3x3();
    }

    static constexpr matrix3x3 rotate(T a) noexcept
    {
        T s = std::sin(a);
        T c = std::cos(a);

        matrix3x3 m;

        m.m_11 = c;
        m.m_12 = s;
        m.m_13 = 0;
        m.m_21 = -s;
        m.m_22 = c;
        m.m_23 = 0;
        m.m_31 = 0;
        m.m_32 = 0;
        m.m_33 = 1;

        return m;
    }

    static constexpr matrix3x3 translate(T x, T y) noexcept
    {
        matrix3x3 m;

        m.m_11 = 1;
        m.m_12 = 0;
        m.m_13 = x;
        m.m_21 = 0;
        m.m_22 = 1;
        m.m_23 = y;
        m.m_31 = 0;
        m.m_32 = 0;
        m.m_33 = 1;

        return m;
    }

    static constexpr matrix3x3 translate(vector2<T> t) noexcept
    {
        return translate(t.x, t.y);
    }

    static constexpr matrix3x3 scale(T sx, T sy) noexcept
    {
        matrix3x3 m;

        m.m_11 = sx;
        m.m_12 = 0;
        m.m_13 = 0;
        m.m_21 = 0;
        m.m_22 = sy;
        m.m_23 = 0;
        m.m_31 = 0;
        m.m_32 = 0;
        m.m_33 = 1;

        return m;
    }

    static constexpr matrix3x3 scale(vector2<T> s) noexcept
    {
        return scale(s.x, s.y);
    }

    void setTranslation(vector2<T> t) noexcept
    {
        m_13 = t.x;
        m_23 = t.y;
    }

    void setTranslation(T x, T y) noexcept
    {
        m_13 = x;
        m_23 = y;
    }

    void setTranslationX(T x) noexcept
    {
        m_13 = x;
    }

    void setTranslationY(T y) noexcept
    {
        m_23 = y;
    }

    void translateBy(vector2<T> t) noexcept
    {
        m_13 += t.x;
        m_23 += t.y;
    }

    void translateBy(T dx, T dy) noexcept
    {
        m_13 += dx;
        m_23 += dy;
    }

    static constexpr matrix3x3 affineTransform(T tx = 0.0f, T ty = 0.0f, T sx = 1.0f, T sy = 1.0f, T rsa = 0.0f, T rpa = 0.0f, vector2<T> rp = {}) noexcept
    {
        if(std::abs(rpa) > std::numeric_limits<T>::epsilon())
        {
            return matrix3x3::translate(tx - rp.x, ty - rp.y) * matrix3x3::rotate(rpa) *
                   matrix3x3::translate(rp.x, rp.y) * matrix3x3::rotate(rsa) * matrix3x3::scale(sx, sy);
        }
        else
        {
            return matrix3x3::scale(sx, sy) * matrix3x3::rotate(rsa) * matrix3x3::translate(tx, ty);
        }
    }

    static constexpr matrix3x3 invAffineTransform(T tx = 0.0f, T ty = 0.0f, T sx = 1.0f, T sy = 1.0f, T rsa = 0.0f, T rpa = 0.0f, vector2<T> rp = {}) noexcept
    {
        if(std::abs(rpa) > std::numeric_limits<T>::epsilon())
        {
            //TODO
            return identity();
        }
        else
        {
            return matrix3x3::translate(-tx, -ty) * matrix3x3::rotate(-rsa) *  matrix3x3::scale(1.0f / sx, 1.0f / sy);
        }
    }

    constexpr matrix3x3 operator*(const matrix3x3& rhs)
    {
        matrix3x3 res;

        res.m_11 =  m_11 * rhs.m_11 + m_21 * rhs.m_12 + m_31 * rhs.m_13;
        res.m_12 =  m_12 * rhs.m_11 + m_22 * rhs.m_12 + m_32 * rhs.m_13;
        res.m_13 =  m_13 * rhs.m_11 + m_23 * rhs.m_12 + m_33 * rhs.m_13;

        res.m_21 =  m_11 * rhs.m_21 + m_21 * rhs.m_22 + m_31 * rhs.m_23;
        res.m_22 =  m_12 * rhs.m_21 + m_22 * rhs.m_22 + m_32 * rhs.m_23;
        res.m_23 =  m_13 * rhs.m_21 + m_23 * rhs.m_22 + m_33 * rhs.m_23;

        res.m_31 =  m_11 * rhs.m_31 + m_21 * rhs.m_32 + m_31 * rhs.m_33;
        res.m_32 =  m_12 * rhs.m_31 + m_22 * rhs.m_32 + m_32 * rhs.m_33;
        res.m_33 =  m_13 * rhs.m_31 + m_23 * rhs.m_32 + m_33 * rhs.m_33;

        return res;
    }

    constexpr void storeColumns(vector3<T>& col0, vector3<T>& col1, vector3<T>& col2) const
    {
        col0 = {m_11, m_12, m_13};
        col1 = {m_21, m_22, m_23};
        col2 = {m_31, m_32, m_33};
    }

private:
    T m_11, m_12, m_13
    , m_21, m_22, m_23
    , m_31, m_32, m_33;
};

template<class T>
constexpr vector3<T> operator*(const vector3<T>& v, const matrix3x3<T>& m)
{
    return vector3<T>(
                v.x * m.m_11 + v.y * m.m_12 + v.z * m.m_13,
                v.x * m.m_21 + v.y * m.m_22 + v.z * m.m_23,
                v.x * m.m_31 + v.y * m.m_32 + v.z * m.m_33
                );

}

using mat3x3 = matrix3x3<float>;
using dmat3x3 = matrix3x3<double>;
using i16mat3x3 = matrix3x3<int16_t>;
using u16mat3x3 = matrix3x3<uint16_t>;
using i32mat3x3 = matrix3x3<int32_t>;
using u32mat3x3 = matrix3x3<uint32_t>;
using imat3x3 = matrix3x3<int32_t>;
using umat3x3 = matrix3x3<uint32_t>;
using i64mat3x3 = matrix3x3<int64_t>;
using u64mat3x3 = matrix3x3<uint64_t>;

/*------------------------------------------ vector2 ---------------------------------------------*/

template<class T>
class vector2
{
public:
    constexpr vector2() = default;
    constexpr vector2(T s) : x(s), y(s) {}
    constexpr vector2(T _x, T _y) : x(_x), y(_y) {}
    constexpr vector2(const vector3<T>& v3) : x(v3.x), y(v3.y) {}

    constexpr vector2<T> operator-() const noexcept
    {
        return vector2<T>(-x, -y);
    }

    constexpr vector2<T> operator+(const vector2<T>& rhs) const noexcept
    {
        return vector2<T>(x + rhs.x, y + rhs.y);
    }

    constexpr void operator+=(const vector2<T>& rhs) noexcept
    {
       x += rhs.x;
       y += rhs.y;
    }

    constexpr vector2<T> operator-(const vector2<T>& rhs) const noexcept
    {
        return vector2<T>(x - rhs.x, y - rhs.y);
    }

    constexpr void operator-=(const vector2<T>& rhs) noexcept
    {
       x -= rhs.x;
       y -= rhs.y;
    }

    constexpr vector2<T> operator*(const vector2<T>& rhs) const noexcept
    {
        return vector2<T>(x * rhs.x, y * rhs.y);
    }

    constexpr void operator*=(const vector2<T>& rhs) noexcept
    {
       x *= rhs.x;
       y *= rhs.y;
    }

    constexpr vector2<T> operator/(const vector2<T>& rhs) const noexcept
    {
        return vector2<T>(x / rhs.x, y / rhs.y);
    }

    constexpr void operator*=(T scalar)
    {
        x *= scalar;
        y *= scalar;
    }

    constexpr void operator/=(T scalar)
    {
        x /= scalar;
        y /= scalar;
    }

    constexpr T length() const
    {
        return std::sqrt(x*x + y*y);
    }

    T x;
    T y;
};

template<class V, class S>
constexpr vector2<V> operator*(const vector2<V>& v, S scalar)
{
    return vector2<V>(v.x * scalar, v.y * scalar);
}

template<class V, class S>
constexpr vector2<V> operator*(S scalar, const vector2<V>& v)
{
    return vector2<V>(v.x * scalar, v.y * scalar);
}

template<class V, class S>
constexpr vector2<V> operator/(const vector2<V>& v, S scalar)
{
    return vector2<V>(v.x / scalar, v.y / scalar);
}

template<class V, class S>
constexpr vector2<V> operator/(S scalar, const vector2<V>& v)
{
    return vector2<V>(v.x / scalar, v.y / scalar);
}

template<class V1, class V2>
constexpr auto dot(const vector2<V1>& lhs, const vector2<V2>& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

template<class V1, class V2>
constexpr auto cross(const vector2<V1>& lhs, const vector2<V2>& rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

using vec2 = vector2<float>;
using dvec2 = vector2<double>;
using i16vec2 = vector2<int16_t>;
using u16vec2 = vector2<uint16_t>;
using i32vec2 = vector2<int32_t>;
using u32vec2 = vector2<uint32_t>;
using ivec2 = vector2<int32_t>;
using uvec2 = vector2<uint32_t>;
using i64vec2 = vector2<int64_t>;
using u64vec2 = vector2<uint64_t>;

/*------------------------------------------ vector3 ---------------------------------------------*/

template<class T>
class vector3
{
public:
    constexpr vector3() = default;
    constexpr vector3(T s) : x(s), y(s), z(s) {}
    constexpr vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
    constexpr vector3(const vector2<T>& v2, T _z) : x(v2.x), y(v2.y), z(_z) {}

    constexpr vector3<T> operator-() const noexcept
    {
        return vector3<T>(-x, -y, -z);
    }

    constexpr vector3<T> operator+(const vector3<T>& rhs) const noexcept
    {
        return vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    constexpr void operator+=(const vector3<T>& rhs) noexcept
    {
       x += rhs.x;
       y += rhs.y;
       z += rhs.z;
    }

    constexpr vector3<T> operator-(const vector3<T>& rhs) const noexcept
    {
        return vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    constexpr void operator-=(const vector3<T>& rhs) noexcept
    {
       x -= rhs.x;
       y -= rhs.y;
       z -= rhs.z;
    }

    constexpr vector3<T> operator*(const vector3<T>& rhs) const noexcept
    {
        return vector3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    constexpr void operator*=(const vector3<T>& rhs) noexcept
    {
       x *= rhs.x;
       y *= rhs.y;
       z *= rhs.z;
    }

    constexpr vector3<T> operator/(const vector3<T>& rhs) const noexcept
    {
        return vector3<T>(x / rhs.x, y / rhs.y, z / rhs.z);
    }

    constexpr void operator*=(T scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
    }

    constexpr void operator/=(T scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
    }

    constexpr T length() const
    {
        return std::sqrt(x*x + y*y + z*z);
    }

    T x;
    T y;
    T z;
};

template<class V, class S>
constexpr vector3<V> operator*(const vector3<V>& v, S scalar)
{
    return vector3<V>(v.x * scalar, v.y * scalar, v.z * scalar);
}

template<class V, class S>
constexpr vector3<V> operator*(S scalar, const vector3<V>& v)
{
    return vector3<V>(v.x * scalar, v.y * scalar, v.z * scalar);
}

template<class V, class S>
constexpr vector3<V> operator/(const vector3<V>& v, S scalar)
{
    return vector3<V>(v.x / scalar, v.y / scalar, v.z / scalar);
}

template<class V, class S>
constexpr vector3<V> operator/(S scalar, const vector3<V>& v)
{
    return vector3<V>(v.x / scalar, v.y / scalar, v.z / scalar);
}

template<class V1, class V2>
constexpr auto dot(const vector3<V1>& lhs, const vector3<V2>& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template<class V1, class V2>
constexpr auto cross(const vector3<V1>& lhs, const vector3<V2>& rhs)
{
    //@TODO: implement
    return 0;//lhs.x * rhs.y - lhs.y * rhs.x;
}

using vec3 = vector3<float>;
using dvec3 = vector3<double>;
using i16vec3 = vector3<int16_t>;
using u16vec3 = vector3<uint16_t>;
using i32vec3 = vector3<int32_t>;
using u32vec3 = vector3<uint32_t>;
using ivec3 = vector3<int32_t>;
using uvec3 = vector3<uint32_t>;
using i64vec3 = vector3<int64_t>;
using u64vec3 = vector3<uint64_t>;

using mat4x4 = int;
#endif

struct Frustum
{
    std::array<vec4, 6> planes;
};

constexpr vec3 sphericalToCartesian(float r, float theta, float phi)
{
    return r * vec3(std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta));
}

#endif // GEOMETRY_H
