#include "Algorithms.hpp"
#include "platypus/core/Application.hpp"
#include <cmath>


namespace platypus
{
    Vector2f screen_to_ndc(int screenX, int screenY)
    {
        const Window& window = Application::get_instance()->getWindow();
        float windowWidth = static_cast<float>(window.getWidth());
        float windowHeight = static_cast<float>(window.getHeight());
        return { (screenX / windowWidth) * 2.0f - 1.0f,
                (screenY / windowHeight) * 2.0f - 1.0f };
    }


    // Converts screen coordinate to "3d world coordinate"
    // NOTE: Requires Application and window to exist!
    Vector3f screen_to_world_space(
        int screenX,
        int screenY,
        const Matrix4f& projMat,
        const Matrix4f& viewMat
    )
    {
        const Window& window = Application::get_instance()->getWindow();
        float windowWidth = static_cast<float>(window.getWidth());
        float windowHeight = static_cast<float>(window.getHeight());

        float screenXf = static_cast<float>(screenX);
        float screenYf = static_cast<float>(screenY);

        // NOTE: ISSUE!? Not sure, should flip depending on platform!?
        float ndcX = (screenXf / windowWidth) * 2.0f - 1.0f;
        float ndcY = (screenYf / windowHeight) * 2.0f - 1.0f;

        Vector4f clipCoords(ndcX, ndcY, -1.0f, 1.0f);
        Matrix4f invProjMat = projMat.inverse();
        Vector4f cameraSpace = invProjMat * clipCoords;
        cameraSpace.z = -1.0f;
        cameraSpace.w = 0.0f;

        Matrix4f invViewMat = viewMat.inverse();
        Vector4f worldSpace = invViewMat * cameraSpace;
        worldSpace.w = 0.0f;
        Vector4f normalizedWorldSpace = worldSpace.normalize();

        return
        {
            normalizedWorldSpace.x,
            normalizedWorldSpace.y,
            normalizedWorldSpace.z
        };
    }


    // Returns next closest power of 2 value from v
    unsigned int get_next_pow2(unsigned int v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }


    float get_distance(const Vector3f& left, const Vector3f& right)
    {
        return sqrtf(((left.x - right.x) * (left.x - right.x)) + ((left.y - right.y) * (left.y - right.y)) + ((left.z - right.z) * (left.z - right.z)));
    }


    // Got this from:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection.html
    bool ray_polygon_intersect_MOLLER_TRUMBORE(
        const Vector3f &orig, const Vector3f &dir,
        const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
        float &t, float &u, float &v
    )
    {
        bool allowCulling = true;
        const float kEpsilon = 0.00025f;

        Vector3f v0v1 = v1 - v0;
        Vector3f v0v2 = v2 - v0;
        Vector3f pvec = dir.cross(v0v2);
        float det = v0v1.dotp(pvec);
        if (allowCulling)
        {
            // if the determinant is negative, the triangle is 'back facing'
            // if the determinant is close to 0, the ray misses the triangle
            if (det < kEpsilon) return false;
        }
        else
        {
            // ray and triangle are parallel if det is close to 0
            if (fabs(det) < kEpsilon) return false;
        }

        float invDet = 1 / det;
        Vector3f tvec = orig - v0;
        u = tvec.dotp(pvec) * invDet;
        if (u < 0 || u > 1) return false;

        Vector3f qvec = tvec.cross(v0v1);
        v = dir.dotp(qvec) * invDet;
        if (v < 0 || u + v > 1) return false;

        t = v0v2.dotp(qvec) * invDet;

        return true;
    }

    bool ray_polygon_intersect(
        const Vector3f &orig,
        const Vector3f &dir,
        const Vector3f& colliderPos,
        const Vector3f& colliderScale,
        float &t, float &u, float &v
    )
    {
        // Figure out "imaginary" box vertices forming triangles to collide with
        Vector3f vertices[8];

        /*
            Each face is like this. "Top left" and "bottom right" vertices
            -----
            |  /|
            | / |
            |/  |
            -----
        */

        // "front face"
        // tri 1 (top left)
        vertices[0] = { colliderPos.x + colliderScale.x, colliderPos.y + colliderScale.y, colliderPos.z + colliderScale.z };
        vertices[1] = { colliderPos.x - colliderScale.x, colliderPos.y + colliderScale.y, colliderPos.z + colliderScale.z };
        vertices[2] = { colliderPos.x - colliderScale.x, colliderPos.y - colliderScale.y, colliderPos.z + colliderScale.z };
        // tri 2 (bottom right)
        // vertices[2]
        vertices[3] = { colliderPos.x + colliderScale.x, colliderPos.y - colliderScale.y, colliderPos.z + colliderScale.z };
        // vertices[0]

        // "back face" (as like looking straight at it towards +z from -z direction)
        // tri 1 (top left)
        vertices[4] = { colliderPos.x - colliderScale.x, colliderPos.y + colliderScale.y, colliderPos.z - colliderScale.z };
        vertices[5] = { colliderPos.x + colliderScale.x, colliderPos.y + colliderScale.y, colliderPos.z - colliderScale.z };
        vertices[6] = { colliderPos.x + colliderScale.x, colliderPos.y - colliderScale.y, colliderPos.z - colliderScale.z };
        // tri 2 (bottom right)
        // vertices[6]
        vertices[7] = { colliderPos.x - colliderScale.x, colliderPos.y - colliderScale.y, colliderPos.z - colliderScale.z };
        // vertices[4]

        // "right face"
        // tri 1
        // vertices[5]
        // vertices[0]
        // vertices[3]
        // tri 2
        // vertices[3]
        // vertices[6]
        // vertices[5]

        // "left face"
        // tri 1
        // vertices[1]
        // vertices[4]
        // vertices[7]
        // tri 2
        // vertices[7]
        // vertices[2]
        // vertices[1]

        // "top face"
        // tri 1
        // vertices[5]
        // vertices[4]
        // vertices[1]
        // tri 2
        // vertices[1]
        // vertices[0]
        // vertices[5]

        // NOTE: Not using "bottom face" atm

        // tot 10 tris
        const int triangleCount = 10;
        int triangleIndices[30] =
        {
            0, 1, 2,
            2, 3, 0,
            4, 5, 6,
            6, 7, 4,
            5, 0, 3,
            3, 6, 5,
            1, 4, 7,
            7, 2, 1,
            5, 4, 1,
            1, 0, 5
        };

        for (int i = 0; i < 30; i += 3)
        {
            const Vector3f& v0 = vertices[triangleIndices[i]];
            const Vector3f& v1 = vertices[triangleIndices[i + 1]];
            const Vector3f& v2 = vertices[triangleIndices[i + 2]];
            bool intersect = ray_polygon_intersect_MOLLER_TRUMBORE(
                orig, dir,
                v0, v1, v2,
                t,u, v
            );
            if (intersect)
                return true;
        }
        return false;
    }
}
