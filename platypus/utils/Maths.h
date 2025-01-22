#pragma once

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
    };
}
