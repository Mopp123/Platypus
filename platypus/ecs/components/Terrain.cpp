#include "Terrain.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    Terrain* create_terrain(
        entityID_t target,
        float tileSize,
        size_t verticesPerRow,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        ComponentType componentType = ComponentType::COMPONENT_TYPE_TERRAIN;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Terrain component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        Terrain* pTerrain = reinterpret_cast<Terrain*>(pComponent);
        pTerrain->tileSize = tileSize;
        pTerrain->verticesPerRow = verticesPerRow;
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        return pTerrain;
    }

    float get_triangle_height_barycentric(
        const platypus::Vector3f& p1,
        const platypus::Vector3f& p2,
        const platypus::Vector3f& p3,
        const platypus::Vector2f& pos
    )
    {
        float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
        float l1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
        float l2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
        float l3 = 1.0f - l1 - l2;
        return l1 * p1.y + l2 * p2.y + l3 * p3.y;
    }

    float get_terrain_height(
        Mesh* pTerrainMesh,
        Terrain* pTerrainComponent,
        Transform* pTerrainTransform,
        float worldX,
        float worldZ
    )
    {
        const float tileSize = pTerrainComponent->tileSize;
        const size_t verticesPerRow = pTerrainComponent->verticesPerRow;

        // Pos relative to terrain
        const Matrix4f& transformationMatrix = pTerrainTransform->globalMatrix;

        const float terrainWorldX = transformationMatrix[0 + 3 * 4];
        const float terrainWorldZ = transformationMatrix[2 + 3 * 4];

        float terrainX = worldX - terrainWorldX;
        float terrainZ = worldZ - terrainWorldZ;

        int gridX = static_cast<int>(std::floor(terrainX / tileSize));
        int gridZ = static_cast<int>(std::floor(terrainZ / tileSize));

        if (gridX < 0 || gridX + 1 >= verticesPerRow || gridZ < 0 || gridZ + 1 >= verticesPerRow)
        {
            return 0.0f;
        }

        // Coordinates in relation to the current tile, in range 0 to 1
        float tileSpaceX = std::fmod(terrainX, tileSize) / tileSize;
        float tileSpaceZ = std::fmod(terrainZ, tileSize) / tileSize;

        // NOTE: WARNING! This atm only works because all vertex buffers used for
        // rendering has vertex positions first in the buffer!
        const Buffer* pVertexBuffer = pTerrainMesh->getVertexBuffer();
        const VertexBufferLayout& vertexBufferLayout = pTerrainMesh->getVertexBufferLayout();
        const size_t bufferElementCount = vertexBufferLayout.getElements().size();

        const float* pBufferData = reinterpret_cast<const float*>(pVertexBuffer->getData());
        float currentHeight = pBufferData[((gridX + gridZ * verticesPerRow) + 1) * bufferElementCount];
        float rightHeight = pBufferData[(((gridX + 1) + gridZ * verticesPerRow) + 1) * bufferElementCount];
        float bottomHeight = pBufferData[((gridX + (gridZ + 1) * verticesPerRow) + 1) * bufferElementCount];
        float bottomRightHeight = pBufferData[(((gridX + 1) + (gridZ + 1) * verticesPerRow) + 1) * bufferElementCount];

        // Check which triangle of the tile we are standing on..
        if (tileSpaceX <= tileSpaceZ) {
            return get_triangle_height_barycentric(
                //Vector3f(0, _heightmap[gridX + gridZ * _terrainVerticesPerRow], 0),
                //Vector3f(0, _heightmap[gridX + (gridZ + 1) * _terrainVerticesPerRow], 1),
                //Vector3f(1, _heightmap[(gridX + 1) + (gridZ + 1) * _terrainVerticesPerRow], 1),

                Vector3f(0, currentHeight, 0),
                Vector3f(0, bottomHeight, 1),
                Vector3f(1, bottomRightHeight, 1),
                Vector2f(tileSpaceX, tileSpaceZ));
        }
        else {
            return get_triangle_height_barycentric(
                //Vector3f(0, _heightmap[gridX + gridZ * _terrainVerticesPerRow], 0),
                //Vector3f(1, _heightmap[(gridX + 1) + (gridZ + 1) * _terrainVerticesPerRow], 1),
                //Vector3f(1, _heightmap[(gridX + 1) + gridZ * _terrainVerticesPerRow], 0),

                Vector3f(0, currentHeight, 0),
                Vector3f(1, bottomRightHeight, 1),
                Vector3f(1, rightHeight, 0),

                Vector2f(tileSpaceX, tileSpaceZ));
        }
    }

    std::vector<char> serialize(const Terrain* pTerrain)
    {
        std::vector<char> serializedData(serialized_terrain_size);
        char* pBuf = serializedData.data();
        ComponentType componentType = ComponentType::COMPONENT_TYPE_TERRAIN;
        memcpy(pBuf, &componentType, sizeof(ComponentType));
        size_t pos = sizeof(ComponentType);

        memcpy(pBuf + pos, &pTerrain->tileSize, sizeof(float));
        pos += sizeof(float);

        const uint32_t verticesPerRow = static_cast<uint32_t>(pTerrain->verticesPerRow);
        memcpy(pBuf + pos, &verticesPerRow, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        PLATYPUS_ASSERT(pos == serialized_terrain_size);
        return serializedData;
    }

    void deserialize(
        Scene* pScene,
        Terrain** ppTerrain,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_terrain_size);

        const char* pBuf = reinterpret_cast<const char*>(pData);

        ComponentType componentType;
        memcpy(&componentType, pBuf, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_TERRAIN);
        size_t pos = sizeof(ComponentType);

        float tileSize = 0.0f;
        memcpy(&tileSize, pBuf + pos, sizeof(float));
        pos += sizeof(float);

        uint32_t verticesPerRow = 0;
        memcpy(&verticesPerRow, pBuf + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        PLATYPUS_ASSERT(pos == serialized_terrain_size);

        *ppTerrain = create_terrain(
            entityID,
            tileSize,
            static_cast<size_t>(verticesPerRow),
            pScene,
            true
        );
    }
}
