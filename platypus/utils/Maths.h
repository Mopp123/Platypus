#pragma once

#include <string>
#include <cstring>

#define PLATY_MATH_PI 3.14159265358979323846


namespace platypus
{
    class Vector2f
    {
    public:
        union
        {
            struct
            {
                float x = 0.0f;
                float y = 0.0f;
            };
            struct
            {
                float r;
                float g;
            };
        };

        Vector2f() {}
        Vector2f(float x, float y) : x(x), y(y) {}
        Vector2f(const Vector2f& other) : x(other.x), y(other.y) {}

        float dotp(const Vector2f& other) const;
        float length() const;
        Vector2f normalize() const;

        Vector2f operator+(const Vector2f& other) const;
        Vector2f operator*(const Vector2f& other) const;
        Vector2f operator*(float value) const;

        std::string toString() const;
    };


    class Vector3f : public Vector2f
    {
    public:
        union
        {
            float z = 0.0f;
            float b;
        };

        Vector3f() {}
        Vector3f(float x, float y, float z) : Vector2f(x, y), z(z) {}
        Vector3f(const Vector3f& other) : Vector2f(other.x, other.y), z(other.z) {}

        float dotp(const Vector3f& other) const;
        float length() const;
        Vector3f normalize() const;
        Vector3f cross(const Vector3f& other) const;
        Vector3f lerp(const Vector3f& other, float amount) const;

        Vector3f operator+(const Vector3f& other) const;
        Vector3f operator-(const Vector3f& other) const;
        Vector3f operator*(const Vector3f& other) const;
        Vector3f operator*(float value) const;

        std::string toString() const;
    };


    class Vector4f : public Vector3f
    {
    public:
        union
        {
            float w = 0.0f;
            float a;
        };

        Vector4f() {};
        Vector4f(float x, float y, float z, float w) : Vector3f(x, y, z), w(w) {}
        Vector4f(const Vector4f& other) : Vector3f(other.x, other.y, other.z), w(other.w) {}

        float length() const;
        Vector4f normalize() const;

        Vector4f operator+(const Vector4f& other) const;
        Vector4f operator*(const Vector4f& other) const;
        Vector4f operator*(float value) const;

        bool operator==(const Vector4f other) const;
        bool operator!=(const Vector4f other) const;

        std::string toString() const;
    };


    class Matrix4f
    {
    private:
        float _data[16];

    public:
        Matrix4f();
        Matrix4f(float diag);
        void setIdentity();
        Matrix4f inverse() const;
        void operator=(const Matrix4f& other);
        friend Matrix4f operator*(const Matrix4f& left, const Matrix4f& right);
        friend Vector4f operator*(const Matrix4f& left, const Vector4f& right);

        std::string toString() const;

        inline float& operator[](int index) { return _data[index]; }
        inline float operator[](int index) const { return _data[index]; }
        inline const float* getRawArray() const { return _data; }
        inline bool operator==(const Matrix4f& other) const { return memcmp(_data, other._data, sizeof(float) * 16) == 0; }
    };


    class Quaternion
    {
    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;

        Quaternion() {}
        Quaternion(const Vector3f& axis, float angle);
        Quaternion(float x, float y, float z, float w) :
            x(x), y(y), z(z), w(w)
        {}

        Quaternion(const Quaternion& other) :
            x(other.x), y(other.y), z(other.z), w(other.w)
        {}

        float dotp(const Quaternion& other) const;
        float length() const;
        Quaternion normalize() const;
        Quaternion conjugate() const;
        Matrix4f toRotationMatrix() const;
        Quaternion slerp(const Quaternion& other, float amount) const;

        Quaternion operator+(const Quaternion& other) const;
        Quaternion operator-(const Quaternion& other) const;
        Quaternion operator*(const Quaternion& other) const;
        Quaternion operator*(float other) const;

        std::string toString() const;
    };


    Matrix4f create_transformation_matrix(
        const Vector3f& pos,
        const Quaternion& rotation,
        const Vector3f& scale
    );

    Matrix4f create_rotation_matrix(float pitch, float yaw);

    // NOTE: Some issues with clipspace z component, differing with OpenGL and Vulkan!
    Matrix4f create_orthographic_projection_matrix(
        float left,
        float right,
        float top,
        float bottom,
        float zNear,
        float zFar
    );

    // NOTE: Some issues with clipspace z component, differing with OpenGL and Vulkan!
    Matrix4f create_perspective_projection_matrix(
        float aspectRatio,
        float fov,
        float zNear, float zFar
    );
}
