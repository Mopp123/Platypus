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

    size_t get_serialized_entity_size(const Scene* pScene, const Entity& entity)
    {
        const std::string name = pScene->getEntityName(entity.id);
        return serialized_entity_base_size + name.size();
    }

    std::vector<char> serialize_entity(
        const Scene* pScene,
        const Entity& entity
    )
    {
        const size_t serializedSize = get_serialized_entity_size(pScene, entity);
        std::vector<char> serializedData(serializedSize);
        char* pBuf = serializedData.data();
        memcpy(
            pBuf,
            &entity.uuid,
            sizeof(UUID_t)
        );
        size_t pos = sizeof(UUID_t);

        memcpy(
            pBuf + pos,
            &entity.componentMask,
            sizeof(uint64_t)
        );
        pos += sizeof(uint64_t);

        memcpy(
            pBuf + pos,
            &entity.active,
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        const std::string name = pScene->getEntityName(entity.id);
        const uint32_t nameSizeU32 = static_cast<const uint32_t>(name.size());
        memcpy(
            pBuf + pos,
            &nameSizeU32,
            sizeof(uint32_t)
        );
        pos += sizeof(uint32_t);

        memcpy(
            pBuf + pos,
            name.data(),
            name.size()
        );
        pos += name.size();
        PLATYPUS_ASSERT(pos == serializedSize);

        return serializedData;
    }


    void deserialize_entity(
        Scene* pScene,
        Entity& outEntity,
        const void* pData
    )
    {
        const char* pBuf = reinterpret_cast<const char*>(pData);
        UUID_t uuid;
        memcpy(
            &uuid,
            pData,
            sizeof(UUID_t)
        );
        size_t pos = sizeof(UUID_t);

        uint64_t componentMask;
        memcpy(
            &componentMask,
            pBuf + pos,
            sizeof(uint64_t)
        );
        pos += sizeof(uint64_t);

        uint8_t active;
        memcpy(
            &active,
            pBuf + pos,
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        uint32_t nameSizeU32 = 0;
        memcpy(
            &nameSizeU32,
            pBuf + pos,
            sizeof(uint32_t)
        );
        pos += sizeof(uint32_t);

        const size_t nameSize = static_cast<const size_t>(nameSizeU32);
        std::string name;
        if (nameSize > 0)
        {
            char* pNameData = new char[nameSize];
            memcpy(
                pNameData,
                pBuf + pos,
                nameSize
            );
            pos += nameSize;
            name = std::string(pNameData, nameSize);
            delete[] pNameData;
        }

        // NOTE: atm assuming that all written entities are in correct order
        //  -> scene assigns the id
        //  TODO: Add names for serialized entities!
        entityID_t entityID = pScene->createEntity(name, uuid);
        pScene->setComponentMask(entityID, componentMask);
        pScene->setEntityActive(entityID, static_cast<bool>(active));
        outEntity = pScene->getEntity(uuid);
        PLATYPUS_ASSERT(pos == get_serialized_entity_size(pScene, outEntity));
    }


    EntityHierarchyManager::EntityHierarchyManager(Scene* pScene) :
        _memoryPool(
            free_range_func,
            this,
            validate_range_func,
            this
        ),
        _pScene(pScene)
    {
    }

    void EntityHierarchyManager::addChild(Children* pChildrenComponent, entityID_t childEntityID)
    {
        const int32_t baseOffset = pChildrenComponent->offset;
        const int32_t newOffset = _memoryPool.add(
            baseOffset,
            getComponentStorageSize(pChildrenComponent->count),
            _elementSize,
            &childEntityID
        );
        PLATYPUS_ASSERT(newOffset != -1);
        pChildrenComponent->offset = newOffset;
        ++pChildrenComponent->count;
    }

    void EntityHierarchyManager::removeChild(Children* pChildrenComponent, entityID_t childEntityID)
    {
        const int32_t currentOffset = pChildrenComponent->offset;
        const size_t currentCount = pChildrenComponent->count;
        PLATYPUS_ASSERT(currentOffset >= 0);

        const size_t unsignedCurrentOffset = static_cast<const size_t>(currentOffset);
        const size_t end = unsignedCurrentOffset + getComponentStorageSize(currentCount);
        std::vector<uint8_t> storage = _memoryPool.accessStorage();
        uint8_t* pStorage = storage.data();
        PLATYPUS_ASSERT(end <= storage.size());

        for (size_t offset = unsignedCurrentOffset; offset < end; offset += _elementSize)
        {
            entityID_t entityID = NULL_ENTITY_ID;
            memcpy(&entityID, pStorage + offset, _elementSize);
            if (entityID == childEntityID)
            {
                //_childrenContainer[i] = NULL_ENTITY_ID;
                memset(pStorage + offset, NULL_ENTITY_ID, _elementSize);

                if (offset == storage.size() - _elementSize)
                {
                    // WTF?
                    PLATYPUS_ASSERT(storage.size() >= _elementSize);

                    //_childrenContainer.pop_back();
                    storage.resize(storage.size() - _elementSize);
                    return;
                }

                // Make all the rest of the child entities IDs be contiguous
                for (size_t remainingOffset = offset; remainingOffset < end; remainingOffset += _elementSize)
                {
                    if (remainingOffset + _elementSize >= end)
                        break;

                    entityID_t nextEntityID = NULL_ENTITY_ID;
                    memcpy(&nextEntityID, pStorage + remainingOffset, _elementSize);
                    const size_t nextOffset = remainingOffset + _elementSize;
                    memcpy(pStorage + nextOffset, &nextEntityID, _elementSize);
                    //_childrenContainer[j] = _childrenContainer[j + 1];

                }
                // TODO: Make sure this never happens!
                PLATYPUS_ASSERT(end >= _elementSize);
                _memoryPool.accessFreeRanges()[end - _elementSize] = _elementSize;
                _memoryPool.packFreeRanges();
                return;
            }
        }

        Debug::log(
            "Failed to find entityID " + std::to_string(childEntityID) + " "
            "from range: " + std::to_string(currentOffset) + " to " + std::to_string(currentOffset + currentCount),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }

    const entityID_t* EntityHierarchyManager::getChildEntityIDs(const Children * const pChildrenComponent) const
    {
        PLATYPUS_ASSERT(pChildrenComponent->offset != -1);
        return reinterpret_cast<const entityID_t*>(
            _memoryPool.accessData(
                pChildrenComponent->offset,
                getComponentStorageSize(pChildrenComponent->count)
            )
        );
    }

    void EntityHierarchyManager::free_range_func(size_t offset, size_t size, void* pUserData)
    {
        EntityHierarchyManager* pEntityHierarchyManager = reinterpret_cast<EntityHierarchyManager*>(pUserData);
        const size_t elemSize = pEntityHierarchyManager->_elementSize;
        size_t count = size / elemSize;
        PLATYPUS_ASSERT(count >= 0);

        for (size_t useOffset = offset; useOffset < offset + size; useOffset += elemSize)
        {
            memset(
                pEntityHierarchyManager->_memoryPool.accessStorage().data() + useOffset,
                NULL_ENTITY_ID,
                elemSize
            );
        }
    }

    bool EntityHierarchyManager::validate_free_range_func(size_t offset, size_t size, void* pUserData)
    {
        EntityHierarchyManager* pEntityHierarchyManager = reinterpret_cast<EntityHierarchyManager*>(pUserData);
        const size_t elemSize = pEntityHierarchyManager->_elementSize;
        size_t count = size / elemSize;
        PLATYPUS_ASSERT(count >= 0);

        for (size_t useOffset = offset; useOffset < offset + size; useOffset += elemSize)
        {
            entityID_t entityID = NULL_ENTITY_ID;
            memcpy(
                &entityID,
                pEntityHierarchyManager->_memoryPool.accessStorage().data() + useOffset,
                elemSize
            );
            if (entityID != NULL_ENTITY_ID)
                return false;
        }
        return true;
    }
}
