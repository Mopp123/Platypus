#pragma once

#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"
#include "Transform.hpp"
#include "platypus/assets/Mesh.hpp"


namespace platypus
{
    constexpr size_t serialized_terrain_size =
        sizeof(ComponentType) +
        sizeof(float) +
        sizeof(uint32_t);

    struct Terrain
    {
        float tileSize = 0.0f;
        size_t verticesPerRow = 0;
    };

    Terrain* create_terrain(
        entityID_t target,
        float tileSize,
        size_t verticesPerRow,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    Vector2i to_terrain_mesh_coords(
        float terrainSpaceX,
        float terrainSpaceZ,
        float tileSize,
        size_t verticesPerRow
    );

    // NOTE: This can ofc be used for many other kinds of things than just
    // getting terrain height..
    // TODO: Maybe change name and put in Algorithms?
    float get_triangle_height_barycentric(
        const Vector3f& p1,
        const Vector3f& p2,
        const Vector3f& p3,
        const Vector2f& pos
    );

    float get_terrain_height(
        const Mesh* pTerrainMesh,
        const Terrain* pTerrainComponent,
        const Transform* pTerrainTransform,
        float worldX,
        float worldZ
    );

    float get_terrain_vertex_height(
        const Mesh* pTerrainMesh,
        Vector2i gridPosition,
        float tileSize,
        size_t verticesPerRow
    );

    void set_terrain_height(
        Mesh* pTerrainMesh,
        Vector2i meshSpacePosition,
        float height,
        float tileSize,
        size_t verticesPerRow
    );

    void update_terrain_normals(
        Mesh* pTerrainMesh,
        float tileSize,
        size_t verticesPerRow
    );

    std::vector<char> serialize(const Terrain* pTerrain);

    void deserialize(
        Scene* pScene,
        Terrain** ppTerrain,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );
}
