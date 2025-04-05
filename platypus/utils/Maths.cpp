#include "Maths.h"
#include "platypus/core/Debug.h"
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

    Vector3f Vector3f::operator+(const Vector3f& other) const
    {
        return { x + other.x, y + other.y, z + other.z };
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

    Vector4f Vector4f::operator*(const Vector4f& other) const
    {
        return { x * other.x, y * other.y, z * other.z, w * other.w };
    }

    Vector4f Vector4f::operator*(float value) const
    {
        return { x * value, y * value, z * value, w * value };
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


    Matrix4f create_rotation_matrix(float pitch, float yaw)
    {
        Matrix4f pitchMatrix(1.0f);
        Matrix4f yawMatrix(1.0f);

        pitchMatrix[1 + 1 * 4] = cos(pitch);
        pitchMatrix[1 + 2 * 4] = -sin(pitch);
        pitchMatrix[2 + 1 * 4] = sin(pitch);
        pitchMatrix[2 + 2 * 4] = cos(pitch);

        yawMatrix[0 + 0 * 4] = cos(yaw);
        yawMatrix[0 + 2 * 4] = sin(yaw);
        yawMatrix[2 + 0 * 4] = -sin(yaw);
        yawMatrix[2 + 2 * 4] = cos(yaw);

        return yawMatrix * pitchMatrix;
    }


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

        result[0] = 2.0f / (right - left);
        result[1 + 1 * 4] = 2.0f / (top - bottom);
        result[2 + 2 * 4] = -2.0f / (zFar - zNear);
        result[3 + 3 * 4] = 1.0f;
        result[0 + 3 * 4] = -((right + left) / (right - left));
        result[1 + 3 * 4] = -((top + bottom) / (top - bottom));
        result[2 + 3 * 4] = -((zFar + zNear) / (zFar - zNear));

        return result;
    }


    Matrix4f create_perspective_projection_matrix(
        float aspectRatio,
        float fov,
        float zNear, float zFar
    )
    {
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
