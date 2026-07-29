#include "Entity.hpp"
#include "components/Renderable.hpp"
#include "components/Transform.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"
#include <cstring>


namespace platypus
{
    std::string entity_error_type_to_string(EntityErrorType error)
    {
        switch (error)
        {
            case EntityErrorType::NO_ERROR: return "";
            case EntityErrorType::COMPONENT_RENDERABLE3D_MESH_UNAVAILABLE: return "Mesh unavailable";
            case EntityErrorType::COMPONENT_RENDERABLE3D_MATERIAL_UNAVAILABLE: return "Material unavailable";
            case EntityErrorType::COMPONENT_RENDERABLE3D_INCOMPATIBLE_MESH_MATERIAL: return "Incompatible Mesh and Material";
        }
    }


    void handle_mesh_unavailable_error(Scene* pScene, EntityError error)
    {
        PLATYPUS_ASSERT(error.type == EntityErrorType::COMPONENT_RENDERABLE3D_MESH_UNAVAILABLE);
        PLATYPUS_ASSERT(error.targetComponents.size() == 1);
        uint32_t componentType = error.targetComponents.begin()->first;
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_RENDERABLE3D);

        Mesh* pErrorMesh = Application::get_instance()->getAssetManager()->getErrorMesh();
        PLATYPUS_ASSERT(pErrorMesh);

        void* pRenderableComponent = error.targetComponents.begin()->second;
        Renderable3D* pRenderable = reinterpret_cast<Renderable3D*>(pRenderableComponent);
        pRenderable->meshID = pErrorMesh->getID();
    }


    void handle_material_unavailable_error(Scene* pScene, EntityError error)
    {
        PLATYPUS_ASSERT(error.type == EntityErrorType::COMPONENT_RENDERABLE3D_MATERIAL_UNAVAILABLE);
        PLATYPUS_ASSERT(error.targetComponents.size() == 1);
        uint32_t componentType = error.targetComponents.begin()->first;
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_RENDERABLE3D);

        Material* pErrorMaterial = Application::get_instance()->getAssetManager()->getErrorMaterial();
        PLATYPUS_ASSERT(pErrorMaterial);

        void* pRenderableComponent = error.targetComponents.begin()->second;
        Renderable3D* pRenderable = reinterpret_cast<Renderable3D*>(pRenderableComponent);
        pRenderable->materialID = pErrorMaterial->getID();
    }


    void handle_incompatible_mesh_material_error(Scene* pScene, EntityError error)
    {
        PLATYPUS_ASSERT(error.type == EntityErrorType::COMPONENT_RENDERABLE3D_INCOMPATIBLE_MESH_MATERIAL);
        PLATYPUS_ASSERT(error.targetComponents.size() == 1);
        uint32_t componentType = error.targetComponents.begin()->first;
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_RENDERABLE3D);

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pErrorMesh = pAssetManager->getErrorMesh();
        Material* pErrorMaterial = pAssetManager->getErrorMaterial();
        PLATYPUS_ASSERT(pErrorMesh);
        PLATYPUS_ASSERT(pErrorMaterial);

        void* pRenderableComponent = error.targetComponents.begin()->second;
        Renderable3D* pRenderable = reinterpret_cast<Renderable3D*>(pRenderableComponent);
        pRenderable->meshID = pErrorMesh->getID();
        pRenderable->materialID = pErrorMaterial->getID();
    }


    Entity::Entity()
    {}

    Entity::Entity(const Entity& other) :
        id(other.id),
        uuid(other.uuid),
        componentMask(other.componentMask),
        active(other.active)
    {}

    void Entity::clear(uint32_t UUIDPoolID)
    {
        id = NULL_ENTITY_ID;
        UUID::erase(uuid, UUIDPoolID);
        componentMask = 0;
    }


    // NOTE: Doesn't work if the mask value's size changes!
    size_t get_component_count(uint64_t componentMask)
    {
        size_t count = 0;
        for (size_t i = 0; i < 64; ++i)
        {
            if (componentMask & (static_cast<uint64_t>(0x1) << i))
                ++count;
        }
        return count;
    }


    std::vector<char> serialize_entity(const Entity& entity, const std::string& entityName)
    {
        size_t nameSize = entityName.size();
        PLATYPUS_ASSERT(nameSize <= serialized_entity_name_size);

        std::vector<char> serializedData(serialized_entity_size);
        memset(serializedData.data(), 0, serialized_entity_size);
        memcpy(
            serializedData.data(),
            &entity.uuid,
            sizeof(UUID_t)
        );
        size_t pos = sizeof(UUID_t);

        memcpy(
            serializedData.data() + pos,
            &entity.componentMask,
            sizeof(uint64_t)
        );
        pos += sizeof(uint64_t);

        memcpy(
            serializedData.data() + pos,
            &entity.active,
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        memcpy(
            serializedData.data() + pos,
            entityName.data(),
            nameSize
        );

        return serializedData;
    }


    void deserialize_entity(
        Scene* pScene,
        Entity& outEntity,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(dataSize == serialized_entity_size);

        UUID_t uuid;
        uint64_t componentMask;
        uint8_t active;
        char name[serialized_entity_name_size];

        memcpy(
            &uuid,
            pData,
            sizeof(UUID_t)
        );
        size_t pos = sizeof(UUID_t);

        memcpy(
            &componentMask,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(uint64_t)
        );
        pos += sizeof(uint64_t);

        memcpy(
            &active,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        memcpy(
            name,
            reinterpret_cast<const char*>(pData) + pos,
            serialized_entity_name_size
        );
        std::string nameStr(name);

        // NOTE: atm assuming that all written entities are in correct order
        //  -> scene assigns the id
        //  TODO: Add names for serialized entities!
        entityID_t entityID = pScene->createEntity(nameStr, uuid);
        pScene->setComponentMask(entityID, componentMask);
        pScene->setEntityActive(entityID, static_cast<bool>(active));
        outEntity = pScene->getEntity(uuid);
    }


    EntityHierarchyManager::EntityHierarchyManager(Scene* pScene) :
        _pScene(pScene)
    {
    }

    size_t EntityHierarchyManager::occupyRange(const std::vector<entityID_t>& childEntities)
    {
        // Check first if suitable free range already exists
        int32_t freeOffset = findFreeRange(childEntities.size());
        size_t useOffset = 0;
        const size_t childCount = childEntities.size();

        if (freeOffset == -1)
        {
            const size_t prevSize = _childrenContainer.size();
            _childrenContainer.resize(prevSize + childCount);
            memcpy(
                _childrenContainer.data() + prevSize,
                childEntities.data(),
                sizeof(entityID_t) * childCount
            );
            useOffset = prevSize;
        }
        else
        {
            useOffset = static_cast<size_t>(freeOffset);
            #ifdef PLATYPUS_DEBUG
            if (!validateFreeRange(useOffset, childCount))
            {
                Debug::log(
                    "Free range validation failed using offset: " + std::to_string(useOffset) + " and count: " + std::to_string(childCount) + " "
                    "Current container length is " + std::to_string(_childrenContainer.size()),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            memcpy(
                _childrenContainer.data() + useOffset,
                childEntities.data(),
                sizeof(entityID_t) * childCount
            );
            _freeRanges.erase(useOffset);
        }

        return useOffset;
    }

    void EntityHierarchyManager::freeRange(Children* pChildren)
    {
        const size_t offset = pChildren->offset;
        const size_t count = pChildren->count;
        if (offset + count > _childrenContainer.size())
        {
            Debug::log(
                "Children component's range (offset = " + std::to_string(offset) + " count = " + std::to_string(count) + ") "
                "out of bounds! EntityHierarchyManager's children container's length is " + std::to_string(_childrenContainer.size()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        for (size_t i = offset; i < offset + count; ++i)
            _childrenContainer[i] = NULL_ENTITY_ID;

        _freeRanges[offset] = count;
    }

    const entityID_t* EntityHierarchyManager::getChildEntities(Children* pChildren) const
    {
        const size_t offset = pChildren->offset;
        const size_t count = pChildren->count;
        if (offset + count > _childrenContainer.size())
        {
            Debug::log(
                "Children component's range (offset = " + std::to_string(offset) + " count = " + std::to_string(count) + ") "
                "out of bounds! EntityHierarchyManager's children container's length is " + std::to_string(_childrenContainer.size()),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _childrenContainer.data() + offset;
    }

    int32_t EntityHierarchyManager::findFreeRange(size_t requiredCount)
    {
        std::unordered_map<size_t, size_t>::const_iterator it;
        for (it = _freeRanges.begin(); it != _freeRanges.end(); ++it)
        {
            if (it->second >= requiredCount)
                return it->first;
        }
        return -1;
    }

    bool EntityHierarchyManager::validateFreeRange(size_t offset, size_t count) const
    {
        if (offset + count > _childrenContainer.size())
            return false;

        for (size_t i = offset; i < offset + count; ++i)
        {
            if (_childrenContainer[i] != NULL_ENTITY_ID)
                return false;
        }
        return true;
    }
}
