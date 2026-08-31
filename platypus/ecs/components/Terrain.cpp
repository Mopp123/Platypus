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

    Vector2i to_terrain_mesh_coords(
        float terrainSpaceX,
        float terrainSpaceZ,
        float tileSize,
        size_t verticesPerRow
    )
    {
        return {
            static_cast<int>(std::floor(terrainSpaceX / tileSize)),
            static_cast<int>(std::floor(terrainSpaceZ / tileSize))
        };
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

        float terrainSpaceX = worldX - terrainWorldX;
        float terrainSpaceZ = worldZ - terrainWorldZ;

        Vector2i gridPos = to_terrain_mesh_coords(terrainSpaceX, terrainSpaceZ, tileSize, verticesPerRow);
        if (gridPos.x < 0 || gridPos.x + 1 >= verticesPerRow || gridPos.y < 0 || gridPos.y + 1 >= verticesPerRow)
        {
            return 0.0f;
        }

        // Coordinates in relation to the current tile, in range 0 to 1
        float tileSpaceX = std::fmod(terrainSpaceX, tileSize) / tileSize;
        float tileSpaceZ = std::fmod(terrainSpaceZ, tileSize) / tileSize;

        // NOTE: WARNING! This atm only works because all vertex buffers used for
        // rendering has vertex positions first in the buffer!
        const Buffer* pVertexBuffer = pTerrainMesh->getVertexBuffer();
        const VertexBufferLayout& vertexBufferLayout = pTerrainMesh->getVertexBufferLayout();
        const size_t stride = vertexBufferLayout.getStride();

        const char* pBufferData = reinterpret_cast<const char*>(pVertexBuffer->getData());
        size_t currentIndex = (gridPos.x + gridPos.y * verticesPerRow) * stride + sizeof(float);
        size_t rightIndex = ((gridPos.x + 1) + gridPos.y * verticesPerRow) * stride + sizeof(float);
        size_t bottomIndex = (gridPos.y + (gridPos.y + 1) * verticesPerRow) * stride + sizeof(float);
        size_t bottomRightIndex = ((gridPos.x + 1) + (gridPos.y + 1) * verticesPerRow) * stride + sizeof(float);

        float currentHeight = 0.0f;
        memcpy(
            &currentHeight,
            pBufferData + currentIndex,
            sizeof(float)
        );

        float rightHeight = 0.0f;
        memcpy(
            &rightHeight,
            pBufferData + rightIndex,
            sizeof(float)
        );

        float bottomHeight = 0.0f;
        memcpy(
            &bottomHeight,
            pBufferData + bottomIndex,
            sizeof(float)
        );
        float bottomRightHeight = 0.0f;
        memcpy(
            &bottomRightHeight,
            pBufferData + bottomRightIndex,
            sizeof(float)
        );

        // Check which triangle of the tile we are standing on..
        if (tileSpaceX <= tileSpaceZ)
        {
            return get_triangle_height_barycentric(
                Vector3f(0, currentHeight, 0),
                Vector3f(0, bottomHeight, 1),
                Vector3f(1, bottomRightHeight, 1),
                Vector2f(tileSpaceX, tileSpaceZ)
            );
        }
        else
        {
            return get_triangle_height_barycentric(
                Vector3f(0, currentHeight, 0),
                Vector3f(1, bottomRightHeight, 1),
                Vector3f(1, rightHeight, 0),
                Vector2f(tileSpaceX, tileSpaceZ)
            );
        }
    }

    void set_terrain_height(
        Mesh* pTerrainMesh,
        Vector2i meshSpacePosition,
        float height,
        float tileSize,
        size_t verticesPerRow
    )
    {
        if (meshSpacePosition.x < 0 || meshSpacePosition.y < 0 ||
            meshSpacePosition.x >= verticesPerRow || meshSpacePosition.y >= verticesPerRow)
        {
            return;
        }

        // NOTE: WARNING! This atm only works because all vertex buffers used for
        // rendering has vertex positions first in the buffer!
        Buffer* pVertexBuffer = pTerrainMesh->getVertexBuffer();
        const VertexBufferLayout& vertexBufferLayout = pTerrainMesh->getVertexBufferLayout();
        const size_t stride = vertexBufferLayout.getStride();
        size_t offset = (meshSpacePosition.x + meshSpacePosition.y * verticesPerRow) * stride + sizeof(float);

        pVertexBuffer->updateDeviceAndHost(
            reinterpret_cast<void*>(&height),
            sizeof(float),
            offset
        );
    }

    void update_terrain_normals(
        Mesh* pTerrainMesh,
        float tileSize,
        size_t verticesPerRow
    )
    {
        Buffer* pVertexBuffer = pTerrainMesh->getVertexBuffer();
        const char* pVertexBufferData = reinterpret_cast<const char*>(pVertexBuffer->getData());
        const VertexBufferLayout& vertexBufferLayout = pTerrainMesh->getVertexBufferLayout();
        const size_t stride = vertexBufferLayout.getStride();

        int32_t normalsBufferIndex = -1;
        size_t normalBufferOffset = 0;
        for (size_t i = 0; i < vertexBufferLayout.getElements().size(); ++i)
        {
            const VertexBufferElement& element = vertexBufferLayout.getElements()[i];
            if (element.getAttribType() == VertexAttributeType::NORMAL)
            {
                normalsBufferIndex = i;
                break;
            }
            normalBufferOffset += get_shader_datatype_size(element.getDataType());
        }
        if (normalsBufferIndex == -1)
        {
            Debug::log(
                "No normals found from Mesh's vertex buffer layout!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        const int32_t signedVerticesPerRow = static_cast<int32_t>(verticesPerRow);
        const int32_t signedStride = static_cast<int32_t>(stride);
        for (int32_t z = 0; z < signedVerticesPerRow; ++z)
        {
            for (int32_t x = 0; x < signedVerticesPerRow; ++x)
            {
                int32_t offset = (x + z * signedVerticesPerRow) * signedStride + sizeof(float);

                // NOTE: SOMETHING's FUCKED HERE!
                // TODO: FIX!
                CONTINUE HERE!
                float leftVertexHeight = 0;
                float rightVertexHeight = 0;
                float downVertexHeight = 0;
                float upVertexHeight = 0;
                int32_t leftVertexBufferOffset = ((x - 1) + z * signedVerticesPerRow) * signedStride + sizeof(float);
                int32_t rightVertexBufferOffset = ((x + 1) + z * signedVerticesPerRow) * signedStride + sizeof(float);
                int32_t upVertexBufferOffset = (x + (z - 1) * signedVerticesPerRow) * signedStride + sizeof(float);
                int32_t downVertexBufferOffset = (x + (z + 1) * signedVerticesPerRow) * signedStride + sizeof(float);

                if (leftVertexBufferOffset >= 0)
                {
                    memcpy(
                        &leftVertexHeight,
                        pVertexBufferData + leftVertexBufferOffset,
                        sizeof(float)
                    );
                }
                if (rightVertexBufferOffset < verticesPerRow)
                {
                    memcpy(
                        &rightVertexHeight,
                        pVertexBufferData + rightVertexBufferOffset,
                        sizeof(float)
                    );
                }

                if (upVertexBufferOffset >= 0)
                {
                    memcpy(
                        &upVertexHeight,
                        pVertexBufferData + upVertexBufferOffset,
                        sizeof(float)
                    );
                }
                if (downVertexBufferOffset < verticesPerRow)
                {
                    memcpy(
                        &downVertexHeight,
                        pVertexBufferData + downVertexBufferOffset,
                        sizeof(float)
                    );
                }
                Vector3f normal(
                    (leftVertexHeight - rightVertexHeight),
                    1.0f,
                    (downVertexHeight - upVertexHeight)
                ); // this is pretty dumb...

                const size_t currentNormalBufferOffset = (static_cast<size_t>(x) + static_cast<size_t>(z) * verticesPerRow) * stride + normalBufferOffset;
                PLATYPUS_ASSERT(currentNormalBufferOffset < pVertexBuffer->getTotalSize());

                pVertexBuffer->updateDeviceAndHost(
                    reinterpret_cast<void*>(&normal),
                    sizeof(Vector3f),
                    currentNormalBufferOffset
                );
            }
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
