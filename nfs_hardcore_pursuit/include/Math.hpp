#pragma once
#include <cmath>
#include <iostream>
#include <algorithm>

namespace Pursuit {
namespace Math {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

inline float Clamp(float val, float minVal, float maxVal) {
    return std::max(minVal, std::min(val, maxVal));
}

inline float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

struct Vec2 {
    float x{0.0f}, y{0.0f};

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    float Dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float LengthSq() const { return x * x + y * y; }
    float Length() const { return std::sqrt(LengthSq()); }

    Vec2 Normalized() const {
        float l = Length();
        return (l > 1e-6f) ? (*this * (1.0f / l)) : Vec2(0, 0);
    }
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }

    Vec3 Cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }

    Vec3 Normalized() const {
        float l = Length();
        return (l > 1e-6f) ? (*this * (1.0f / l)) : Vec3(0, 0, 0);
    }

    static Vec3 Zero() { return {0, 0, 0}; }
    static Vec3 Up() { return {0, 1, 0}; }
    static Vec3 Forward() { return {0, 0, 1}; }
    static Vec3 Right() { return {1, 0, 0}; }
};

inline Vec3 operator*(float s, const Vec3& v) {
    return v * s;
}

struct Mat4 {
    float m[16]{0.0f};

    Mat4() {
        Identity();
    }

    void Identity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 Translation(const Vec3& v) {
        Mat4 res;
        res.m[12] = v.x;
        res.m[13] = v.y;
        res.m[14] = v.z;
        return res;
    }

    static Mat4 RotationY(float radians) {
        Mat4 res;
        float c = std::cos(radians);
        float s = std::sin(radians);
        res.m[0] = c;  res.m[2] = -s;
        res.m[8] = s;  res.m[10] = c;
        return res;
    }

    static Mat4 Perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Mat4 res;
        for (int i = 0; i < 16; ++i) res.m[i] = 0.0f;
        float tanHalfFov = std::tan(fovRad * 0.5f);
        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return res;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 out;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                out.m[c * 4 + r] =
                    m[0 * 4 + r] * o.m[c * 4 + 0] +
                    m[1 * 4 + r] * o.m[c * 4 + 1] +
                    m[2 * 4 + r] * o.m[c * 4 + 2] +
                    m[3 * 4 + r] * o.m[c * 4 + 3];
            }
        }
        return out;
    }

    Vec3 TransformPoint(const Vec3& p) const {
        return {
            m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
            m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
            m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]
        };
    }

    Vec3 TransformVector(const Vec3& v) const {
        return {
            m[0] * v.x + m[4] * v.y + m[8] * v.z,
            m[1] * v.x + m[5] * v.y + m[9] * v.z,
            m[2] * v.x + m[6] * v.y + m[10] * v.z
        };
    }
};

} // namespace Math
} // namespace Pursuit
