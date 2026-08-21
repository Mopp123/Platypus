#pragma once

#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"


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

    std::vector<char> serialize(const Terrain* pTerrain);

    void deserialize(
        Scene* pScene,
        Terrain** ppTerrain,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );
}
