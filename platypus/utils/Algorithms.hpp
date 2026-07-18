#pragma once

#include "Maths.hpp"


namespace platypus
{
    // NOTE: Requires Application and window to exist!
    Vector2f screen_to_ndc(int screenX, int screenY);

    // Converts screen coordinate to "3d world coordinate"
    // NOTE: Requires Application and window to exist!
    Vector3f screen_to_world_space(
        int screenX,
        int screenY,
        const Matrix4f& projMat,
        const Matrix4f& viewMat
    );

    // Returns next closest power of 2 value from v
    unsigned int get_next_pow2(unsigned int v);

    float get_distance(const Vector3f& left, const Vector3f& right);

    bool ray_polygon_intersect_MOLLER_TRUMBORE(
        const Vector3f &orig, const Vector3f &dir,
        const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
        float &t, float &u, float &v
    );

    // *polygon as cube as the args suggests...
    // TODO: Allow this for any polygon
    bool ray_polygon_intersect(
        const Vector3f &orig,
        const Vector3f &dir,
        const Vector3f& colliderPos,
        const Vector3f& colliderScale,
        float &t, float &u, float &v
    );
}
