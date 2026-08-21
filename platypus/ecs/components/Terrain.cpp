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
