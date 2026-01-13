#include "Maths.h"
#include "platypus/core/Debug.hpp"
#include <cstring>
#include <cmath>


namespace platypus
{
    float Vector2f::dotp(const Vector2f& other) const
    {
        return (x * other.x) + (y * other.y);
    }

    float Vector2f::length() const
    {
        return sqrtf((x * x) + (y * y));
    }

    Vector2f Vector2f::normalize() const
    {
        const float l = length();
        return { x / l, y / l };
    }

    Vector2f Vector2f::operator+(const Vector2f& other) const
    {
        return { x + other.x, y + other.y };
    }

    Vector2f Vector2f::operator*(const Vector2f& other) const
    {
        return { x * other.x, y * other.y };
    }

    Vector2f Vector2f::operator*(float value) const
    {
        return { x * value, y * value };
    }

    std::string Vector2f::toString() const
    {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }


    float Vector3f::dotp(const Vector3f& other) const
    {
        return (x * other.x) + (y * other.y) + (z * other.z);
    }

    float Vector3f::length() const
    {
        return sqrtf((x * x) + (y * y) + (z * z));
    }

    Vector3f Vector3f::normalize() const
    {
        const float l = length();
        return { x / l, y / l, z / l };
    }

    Vector3f Vector3f::cross(const Vector3f& other) const
    {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    Vector3f Vector3f::lerp(const Vector3f& other, float amount) const
    {
        const Vector3f& v = *this;
        return v + ((other - v) * amount);
    }

    Vector3f Vector3f::operator+(const Vector3f& other) const
    {
        return { x + other.x, y + other.y, z + other.z };
    }

    Vector3f Vector3f::operator-(const Vector3f& other) const
    {
        return { x - other.x, y - other.y, z - other.z };
    }

    Vector3f Vector3f::operator*(const Vector3f& other) const
    {
        return { x * other.x, y * other.y, z * other.z };
    }

    Vector3f Vector3f::operator*(float value) const
    {
        return { x * value, y * value, z * value };
    }

    std::string Vector3f::toString() const
    {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }


    float Vector4f::length() const
    {
        return sqrtf((x * x) + (y * y) + (z * z) + (w * w));
    }

    Vector4f Vector4f::normalize() const
    {
        const float l = length();
        return { x / l, y / l, z / l, w / l };
    }

    Vector4f Vector4f::operator+(const Vector4f& other) const
    {
        return { x + other.x, y + other.y, z + other.z, w + other.w };
    }

    Vector4f Vector4f::operator-(const Vector4f& other) const
    {
        return { x - other.x, y - other.y, z - other.z, w - other.w };
    }

    Vector4f Vector4f::operator*(const Vector4f& other) const
    {
        return { x * other.x, y * other.y, z * other.z, w * other.w };
    }

    Vector4f Vector4f::operator*(float value) const
    {
        return { x * value, y * value, z * value, w * value };
    }

    bool Vector4f::operator==(const Vector4f other) const
    {
        return memcmp(this, &other, sizeof(Vector4f)) == 0;
    }

    bool Vector4f::operator!=(const Vector4f other) const
    {
        return memcmp(this, &other, sizeof(Vector4f)) != 0;
    }

    std::string Vector4f::toString() const
    {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
    }


    Matrix4f::Matrix4f()
    {
        memset(_data, 0, sizeof(float) * 16);
    }

    Matrix4f::Matrix4f(float diag)
    {
        memset(_data, 0, sizeof(float) * 16);
        for (int i = 0; i < 4; ++i)
        {
            _data[i + i * 4] = diag;
        }
    }

    void Matrix4f::setIdentity()
    {
        memset(_data, 0, sizeof(float) * 16);
        for (int i = 0; i < 4; ++i)
            _data[i + i * 4] = 1.0f;
    }

    // Found from: https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
    //     Comment on the site about this :
    //         "This was lifted from MESA implementation of the GLU library."
    Matrix4f Matrix4f::inverse() const
    {
        Matrix4f inverseMatrix;
        inverseMatrix._data[0] = _data[5] * _data[10] * _data[15] -
            _data[5] * _data[11] * _data[14] -
            _data[9] * _data[6] * _data[15] +
            _data[9] * _data[7] * _data[14] +
            _data[13] * _data[6] * _data[11] -
            _data[13] * _data[7] * _data[10];

        inverseMatrix._data[4] = -_data[4] * _data[10] * _data[15] +
            _data[4] * _data[11] * _data[14] +
            _data[8] * _data[6] * _data[15] -
            _data[8] * _data[7] * _data[14] -
            _data[12] * _data[6] * _data[11] +
            _data[12] * _data[7] * _data[10];

        inverseMatrix._data[8] = _data[4] * _data[9] * _data[15] -
            _data[4] * _data[11] * _data[13] -
            _data[8] * _data[5] * _data[15] +
            _data[8] * _data[7] * _data[13] +
            _data[12] * _data[5] * _data[11] -
            _data[12] * _data[7] * _data[9];

        inverseMatrix._data[12] = -_data[4] * _data[9] * _data[14] +
            _data[4] * _data[10] * _data[13] +
            _data[8] * _data[5] * _data[14] -
            _data[8] * _data[6] * _data[13] -
            _data[12] * _data[5] * _data[10] +
            _data[12] * _data[6] * _data[9];

        inverseMatrix._data[1] = -_data[1] * _data[10] * _data[15] +
            _data[1] * _data[11] * _data[14] +
            _data[9] * _data[2] * _data[15] -
            _data[9] * _data[3] * _data[14] -
            _data[13] * _data[2] * _data[11] +
            _data[13] * _data[3] * _data[10];

        inverseMatrix._data[5] = _data[0] * _data[10] * _data[15] -
            _data[0] * _data[11] * _data[14] -
            _data[8] * _data[2] * _data[15] +
            _data[8] * _data[3] * _data[14] +
            _data[12] * _data[2] * _data[11] -
            _data[12] * _data[3] * _data[10];

        inverseMatrix._data[9] = -_data[0] * _data[9] * _data[15] +
            _data[0] * _data[11] * _data[13] +
            _data[8] * _data[1] * _data[15] -
            _data[8] * _data[3] * _data[13] -
            _data[12] * _data[1] * _data[11] +
            _data[12] * _data[3] * _data[9];

        inverseMatrix._data[13] = _data[0] * _data[9] * _data[14] -
            _data[0] * _data[10] * _data[13] -
            _data[8] * _data[1] * _data[14] +
            _data[8] * _data[2] * _data[13] +
            _data[12] * _data[1] * _data[10] -
            _data[12] * _data[2] * _data[9];

        inverseMatrix._data[2] = _data[1] * _data[6] * _data[15] -
            _data[1] * _data[7] * _data[14] -
            _data[5] * _data[2] * _data[15] +
            _data[5] * _data[3] * _data[14] +
            _data[13] * _data[2] * _data[7] -
            _data[13] * _data[3] * _data[6];

        inverseMatrix._data[6] = -_data[0] * _data[6] * _data[15] +
            _data[0] * _data[7] * _data[14] +
            _data[4] * _data[2] * _data[15] -
            _data[4] * _data[3] * _data[14] -
            _data[12] * _data[2] * _data[7] +
            _data[12] * _data[3] * _data[6];

        inverseMatrix._data[10] = _data[0] * _data[5] * _data[15] -
            _data[0] * _data[7] * _data[13] -
            _data[4] * _data[1] * _data[15] +
            _data[4] * _data[3] * _data[13] +
            _data[12] * _data[1] * _data[7] -
            _data[12] * _data[3] * _data[5];

        inverseMatrix._data[14] = -_data[0] * _data[5] * _data[14] +
            _data[0] * _data[6] * _data[13] +
            _data[4] * _data[1] * _data[14] -
            _data[4] * _data[2] * _data[13] -
            _data[12] * _data[1] * _data[6] +
            _data[12] * _data[2] * _data[5];

        inverseMatrix._data[3] = -_data[1] * _data[6] * _data[11] +
            _data[1] * _data[7] * _data[10] +
            _data[5] * _data[2] * _data[11] -
            _data[5] * _data[3] * _data[10] -
            _data[9] * _data[2] * _data[7] +
            _data[9] * _data[3] * _data[6];

        inverseMatrix._data[7] = _data[0] * _data[6] * _data[11] -
            _data[0] * _data[7] * _data[10] -
            _data[4] * _data[2] * _data[11] +
            _data[4] * _data[3] * _data[10] +
            _data[8] * _data[2] * _data[7] -
            _data[8] * _data[3] * _data[6];

        inverseMatrix._data[11] = -_data[0] * _data[5] * _data[11] +
            _data[0] * _data[7] * _data[9] +
            _data[4] * _data[1] * _data[11] -
            _data[4] * _data[3] * _data[9] -
            _data[8] * _data[1] * _data[7] +
            _data[8] * _data[3] * _data[5];

        inverseMatrix._data[15] = _data[0] * _data[5] * _data[10] -
            _data[0] * _data[6] * _data[9] -
            _data[4] * _data[1] * _data[10] +
            _data[4] * _data[2] * _data[9] +
            _data[8] * _data[1] * _data[6] -
            _data[8] * _data[2] * _data[5];


        float determinant = _data[0] * inverseMatrix._data[0] + _data[1] * inverseMatrix._data[4] + _data[2] * inverseMatrix._data[8] + _data[3] * inverseMatrix._data[12];

        if (determinant == 0)
            return inverseMatrix;

        for (int i = 0; i < 16; ++i)
            inverseMatrix[i] *= (1.0f / determinant);

        return inverseMatrix;
    }

    Matrix4f Matrix4f::transpose() const
    {
        Matrix4f result;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                result[i + j * 4] = _data[j + i * 4];
            }
        }
        return result;
    }

    void Matrix4f::operator=(const Matrix4f& other)
    {
        memcpy(_data, other._data, sizeof(float) * 16);
    }

    Matrix4f operator*(const Matrix4f& left, const Matrix4f& right)
    {
        Matrix4f result;

        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                result[y + x * 4] =
                    left[y + 0 * 4] * right[0 + x * 4] +
                    left[y + 1 * 4] * right[1 + x * 4] +
                    left[y + 2 * 4] * right[2 + x * 4] +
                    left[y + 3 * 4] * right[3 + x * 4];
            }
        }

        return result;
    }

    Vector4f operator*(const Matrix4f& left, const Vector4f& right)
    {
        Vector4f result;
        result.x = left[0 + 0 * 4] * right.x + left[0 + 1 * 4] * right.y + left[0 + 2 * 4] * right.z + left[0 + 3 * 4] * right.w;
        result.y = left[1 + 0 * 4] * right.x + left[1 + 1 * 4] * right.y + left[1 + 2 * 4] * right.z + left[1 + 3 * 4] * right.w;
        result.z = left[2 + 0 * 4] * right.x + left[2 + 1 * 4] * right.y + left[2 + 2 * 4] * right.z + left[2 + 3 * 4] * right.w;
        result.w = left[3 + 0 * 4] * right.x + left[3 + 1 * 4] * right.y + left[3 + 2 * 4] * right.z + left[3 + 3 * 4] * right.w;
        return result;
    }

    std::string Matrix4f::toString() const
    {
        std::string str;
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                if (x == 0)
                    str += "[ ";

                str += std::to_string(_data[y + x * 4]) + " ";

                if (x == 3)
                    str += "]\n";
            }
        }
        return str;
    }


    Quaternion::Quaternion(const Vector3f& axis, float angle)
    {
        float sinHalfAngle = sin(angle / 2.0f);
        x = axis.x * sinHalfAngle;
        y = axis.y * sinHalfAngle;
        z = axis.z * sinHalfAngle;
        w = cos(angle / 2.0f);
    }

    float Quaternion::dotp(const Quaternion& other) const
    {
        return ((x * other.x) + (y * other.y) + (z * other.z) + (w * other.w));
    }

    float Quaternion::length() const
    {
        return sqrtf((x * x) + (y * y) + (z * z) + (w * w));
    }

    Quaternion Quaternion::normalize() const
    {
        const float l = length();
        return { x / l, y / l, z / l, w / l };
    }

    Quaternion Quaternion::conjugate() const
    {
        return { -x, -y, -z, w };
    }

    // Used wikipedia "Quat-derived rotation matrix"
    // This can only be used for "unit quaternion"
    Matrix4f Quaternion::toRotationMatrix() const
    {
        // We force the usage of unit quaternion here...
        Quaternion unitQuat = normalize();

        float s = 1.0f;

        float sqx = unitQuat.x * unitQuat.x;
        float sqy = unitQuat.y * unitQuat.y;
        float sqz = unitQuat.z * unitQuat.z;

        Matrix4f rotationMatrix;

        rotationMatrix[0] = 1.0f - 2 * s * (sqy + sqz);
        rotationMatrix[4] = 2 * s * (unitQuat.x * unitQuat.y - unitQuat.z * unitQuat.w);
        rotationMatrix[8] = 2 * s * (unitQuat.x * unitQuat.z + unitQuat.y * unitQuat.w);

        rotationMatrix[1] = 2 * s * (unitQuat.x * unitQuat.y + unitQuat.z * unitQuat.w);
        rotationMatrix[5] = 1.0f - 2 * s * (sqx + sqz);
        rotationMatrix[9] = 2 * s * (unitQuat.y * unitQuat.z - unitQuat.x * unitQuat.w);

        rotationMatrix[2] = 2 * s * (unitQuat.x * unitQuat.z - unitQuat.y * unitQuat.w);
        rotationMatrix[6] = 2 * s * (unitQuat.y * unitQuat.z + unitQuat.x * unitQuat.w);
        rotationMatrix[10] = 1.0f - 2 * s * (sqx + sqy);

        rotationMatrix[15] = 1.0f;

        return rotationMatrix;
    }

    // Copied from wikipedia : https://en.wikipedia.org/wiki/Slerp
    #define QUATERNION_SLERP__DOT_THRESHOLD 0.9995f
    Quaternion Quaternion::slerp(const Quaternion& other, float amount) const
    {
        // Only unit quaternions are valid rotations.
        // Normalize to avoid undefined behavior.
        Quaternion ua = normalize();
        Quaternion ub = other.normalize();

        // Compute the cosine of the angle between the two vectors.
        float dot = ua.dotp(ub);

        // If the dot product is negative, slerp won't take
        // the shorter path. Note that v1 and -v1 are equivalent when
        // the negation is applied to all four components. Fix by
        // reversing one quaternion.
        if (dot < 0.0f) {
            ub = ub * -1.0f;
            dot = -dot;
        }

        if (dot > QUATERNION_SLERP__DOT_THRESHOLD)
        {
            // If the inputs are too close for comfort, linearly interpolate
            // and normalize the result.
            Quaternion result = ua + ((ub - ua) * amount);
            return result.normalize();
        }

        // Since dot is in range [0, DOT_THRESHOLD], acos is safe
        float theta_0 = acos(dot);        // theta_0 = angle between input vectors
        float theta = theta_0 * amount;   // theta = angle between v0 and result
        float sin_theta = sin(theta);     // compute this value only once
        float sin_theta_0 = sin(theta_0); // compute this value only once

        float s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
        float s1 = sin_theta / sin_theta_0;

        return (ua * s0) + (ub * s1);
    }

    Quaternion Quaternion::operator+(const Quaternion& other) const
    {
        return { x + other.x, y + other.y, z + other.z, w + other.w };
    }

    Quaternion Quaternion::operator-(const Quaternion& other) const
    {
        return { x - other.x, y - other.y, z - other.z, w - other.w };
    }

    Quaternion Quaternion::operator*(const Quaternion& other) const
    {
        Quaternion result;
        result.w = w * other.w - x * other.x - y * other.y - z * other.z;
        result.x = x * other.w + w * other.x + y * other.z - z * other.y;
        result.y = y * other.w + w * other.y + z * other.x - x * other.z;
        result.z = z * other.w + w * other.z + x * other.y - y * other.x;
        return result;
    }

    Quaternion Quaternion::operator*(float other) const
    {
        return { x * other, y * other, z * other, w * other };
    }

    std::string Quaternion::toString() const
    {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
    }


    Matrix4f create_transformation_matrix(
        const Vector3f& pos,
        const Quaternion& rotation,
        const Vector3f& scale
    )
    {
        Matrix4f translationMatrix(1.0f);
        translationMatrix[0 + 3 * 4] = pos.x;
        translationMatrix[1 + 3 * 4] = pos.y;
        translationMatrix[2 + 3 * 4] = pos.z;

        Matrix4f scaleMatrix(1.0f);
        scaleMatrix[0 + 0 * 4] = scale.x;
        scaleMatrix[1 + 1 * 4] = scale.y;
        scaleMatrix[2 + 2 * 4] = scale.z;

        Matrix4f rotationMatrix = rotation.toRotationMatrix();

        // NOTE: Not sure is the order correct...
        return translationMatrix * rotationMatrix * scaleMatrix;
    }


    Matrix4f create_rotation_matrix(float pitch, float yaw, float roll)
    {
        Matrix4f pitchMatrix(1.0f);
        Matrix4f yawMatrix(1.0f);
        Matrix4f rollMatrix(1.0f);

        pitchMatrix[1 + 1 * 4] = cos(pitch);
        pitchMatrix[1 + 2 * 4] = -sin(pitch);
        pitchMatrix[2 + 1 * 4] = sin(pitch);
        pitchMatrix[2 + 2 * 4] = cos(pitch);

        yawMatrix[0 + 0 * 4] = cos(yaw);
        yawMatrix[0 + 2 * 4] = sin(yaw);
        yawMatrix[2 + 0 * 4] = -sin(yaw);
        yawMatrix[2 + 2 * 4] = cos(yaw);

        rollMatrix[0 + 0 * 4] = cos(roll);
        rollMatrix[0 + 1 * 4] = -sin(roll);
        rollMatrix[1 + 0 * 4] = sin(roll);
        rollMatrix[1 + 1 * 4] = cos(roll);

        return yawMatrix * pitchMatrix * rollMatrix;
    }

    Matrix4f create_view_matrix(const Vector3f& position, const Matrix4f& rotationMatrix)
    {
        Matrix4f translationMatrix(1.0f);
        translationMatrix[0 + 3 * 4] = -position.x;
        translationMatrix[1 + 3 * 4] = -position.y;
        translationMatrix[2 + 3 * 4] = -position.z;
        return rotationMatrix * translationMatrix;
    }

    Matrix4f create_view_matrix(const Vector3f& position, const Quaternion& rotation)
    {
        return create_view_matrix(position, rotation.toRotationMatrix());
    }

    // NOTE: Some issues with clipspace z component, differing with OpenGL and Vulkan!
    // https://www.kdab.com/projection-matrices-with-vulkan-part-1/
    Matrix4f create_orthographic_projection_matrix(
        float left,
        float right,
        float top,
        float bottom,
        float zNear,
        float zFar
    )
    {
        Matrix4f result(1.0f);

        // Ment to be used with opengl?
        /*
        result[0] = 2.0f / (right - left);
        result[1 + 1 * 4] = 2.0f / (top - bottom);
        result[2 + 2 * 4] = -2.0f / (zFar - zNear);
        result[3 + 3 * 4] = 1.0f;
        result[0 + 3 * 4] = -((right + left) / (right - left));
        result[1 + 3 * 4] = -((top + bottom) / (top - bottom));
        result[2 + 3 * 4] = -((zFar + zNear) / (zFar - zNear));
        */

        // NOTE: Above was switched to below, using
        // Vulkan-CookBook: https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Library/Source%20Files/10%20Helper%20Recipes/05%20Preparing%20an%20orthographic%20projection%20matrix.cpp
        // which works in the way it was intended. Above commented out requires something like near plane needs to be positive val of negative far plane and
        // the usable range is half the on u put to near and or far planes... it's just fucked... something to do with clipping coords which z is behaving differently between Vulkan and OpenGL
        //
        // BUT WEIRD! For some reason web build is able to extend rendering to infinity towards positive z!
        result[0] = 2.0f / (right - left);
        result[1 + 1 * 4] = 2.0f / (bottom - top);
        result[2 + 2 * 4] = 1.0f / (zNear - zFar);
        result[0 + 3 * 4] = -(right + left) / (right - left);

        result[1 + 3 * 4] = (bottom + top) / (bottom - top);

        result[2 + 3 * 4] = zNear / (zNear - zFar);
        result[3 + 3 * 4] = 1.0f;

        return result;
    }


    // NOTE: Some issues with clipspace z component, differing with OpenGL and Vulkan!
    // https://www.kdab.com/projection-matrices-with-vulkan-part-1/
    Matrix4f create_perspective_projection_matrix(
        float aspectRatio,
        float fov,
        float zNear, float zFar
    )
    {
        if (zNear == 0)
        {
            Debug::log(
                "@create_perspective_projection_matrix "
                "zNear 0 currently unacceptable value! "
                "*At least on Vulkan side, for some reason, doesn't display anything "
                "if zNear = 0... Don't know why..",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        Matrix4f matrix(1.0f);
        matrix[0 + 0 * 4] = 1.0f / (aspectRatio * tan(fov / 2.0f));
        matrix[1 + 1 * 4] = 1.0f / (tan(fov / 2.0f));
        matrix[2 + 2 * 4] = -((zFar + zNear) / (zFar - zNear));
        matrix[3 + 2 * 4] = -1.0f;
        matrix[2 + 3 * 4] = -((2.0f * zFar * zNear) / (zFar - zNear));
        matrix[3 + 3 * 4] = 0.0f;
        return matrix;
    }
}
