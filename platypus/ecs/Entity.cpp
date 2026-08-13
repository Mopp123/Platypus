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
        _pScene(pScene)
    {
    }

    int32_t EntityHierarchyManager::occupyRange(const std::vector<entityID_t>& childEntities)
    {
        // Check first if suitable free range already exists
        int32_t offset = findFreeRange(childEntities.size());
        const size_t childCount = childEntities.size();

        if (offset == -1)
        {
            const size_t prevSize = _childrenContainer.size();
            _childrenContainer.resize(prevSize + childCount);
            memcpy(
                _childrenContainer.data() + prevSize,
                childEntities.data(),
                sizeof(entityID_t) * childCount
            );
            offset = prevSize;
        }
        else
        {
            #ifdef PLATYPUS_DEBUG
            if (!validateFreeRange(offset, childCount))
            {
                Debug::log(
                    "Free range validation failed using offset: " + std::to_string(offset) + " and count: " + std::to_string(childCount) + " "
                    "Current container length is " + std::to_string(_childrenContainer.size()),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            memcpy(
                _childrenContainer.data() + offset,
                childEntities.data(),
                sizeof(entityID_t) * childCount
            );
            _freeRanges.erase(offset);
        }

        return offset;
    }

    void EntityHierarchyManager::freeRange(int32_t offset, size_t count)
    {
        PLATYPUS_ASSERT(offset >= 0);
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

        const size_t unsignedOffset = static_cast<size_t>(offset);
        for (size_t i = unsignedOffset; i < unsignedOffset + count; ++i)
            _childrenContainer[i] = NULL_ENTITY_ID;

        _freeRanges[unsignedOffset] = count;

        packFreeRanges();
    }

    int32_t EntityHierarchyManager::addChild(
        const Children * const pChildren,
        entityID_t childEntityID
    )
    {
        const int32_t currentOffset = pChildren->offset;
        const size_t currentCount = pChildren->count;

        if (currentOffset == -1)
            return occupyRange({ childEntityID });

        // Quickly return using same offset if just adding at the back of the container
        if (currentOffset + currentCount == _childrenContainer.size())
        {
            _childrenContainer.push_back(childEntityID);
            return currentOffset;
        }

        // Quickly return using same offset if can add at empty pos after current range
        // NOTE: BELOW QUITE COMPLICATED, NOT TESTED MIGHT BE FUCKED!!
        // TODO: TEST PROPERLY!
        if (currentCount > 0)
        {
            const size_t nextOffset = currentOffset + currentCount;
            std::map<size_t, size_t>::const_iterator freeIt = _freeRanges.find(nextOffset);
            // Can add at least one more if found from _freeRanges
            if (freeIt != _freeRanges.end())
            {
                PLATYPUS_ASSERT(freeIt->first < _childrenContainer.size());
                PLATYPUS_ASSERT(_childrenContainer[nextOffset] == NULL_ENTITY_ID);
                _childrenContainer[nextOffset] = childEntityID;

                _freeRanges.erase(nextOffset);
                // Update the free offsets
                // If theres more space after the old free offset, "push the cursor forward"
                // with the new free count
                const size_t newFreeCount = freeIt->second - 1;
                if (newFreeCount > 0)
                {
                    const size_t newFreeOffset = nextOffset + 1;
                    if (newFreeOffset < _childrenContainer.size())
                        _freeRanges[nextOffset] = newFreeCount;
                }

                return currentOffset;
            }
        }

        PLATYPUS_ASSERT(currentOffset >= 0);

        const size_t newCount = currentCount + 1;
        std::vector<entityID_t> currentChildren(newCount);
        const entityID_t* pCurrentChildren = getChildEntities(pChildren);
        memcpy(currentChildren.data(), pCurrentChildren, sizeof(entityID_t) * currentCount);

        freeRange(currentOffset, currentCount);
        currentChildren[currentCount] = childEntityID;

        return occupyRange(currentChildren);
    }

    void EntityHierarchyManager::removeChild(
        const Children * const pChildren,
        entityID_t childEntityID
    )
    {
        const int32_t currentOffset = pChildren->offset;
        const size_t currentCount = pChildren->count;
        PLATYPUS_ASSERT(currentOffset >= 0);

        const size_t unsignedCurrentOffset = static_cast<const size_t>(currentOffset);
        const size_t end = unsignedCurrentOffset + currentCount;
        PLATYPUS_ASSERT(end <= _childrenContainer.size());
        for (size_t i = unsignedCurrentOffset; i < end; ++i)
        {
            const entityID_t entityID = _childrenContainer[i];
            if (entityID == childEntityID)
            {
                _childrenContainer[i] = NULL_ENTITY_ID;
                if (i == _childrenContainer.size() - 1)
                {
                    _childrenContainer.pop_back();
                    return;
                }

                // Make all the rest of the child entities IDs be contiguous
                for (size_t j = i; j < end; ++j)
                {
                    if (j + 1 >= end)
                        break;

                    _childrenContainer[j] = _childrenContainer[j + 1];
                }
                _freeRanges[end - 1] = 1;
                packFreeRanges();
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

    const entityID_t* EntityHierarchyManager::getChildEntities(const Children * const pChildren) const
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
        std::map<size_t, size_t>::const_iterator it;
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

    void EntityHierarchyManager::packChildren(size_t beginOffset, size_t freeOffset, size_t count)
    {
        const size_t endOffset = beginOffset + count;
        for (size_t i = freeOffset; i <= endOffset; ++i)
        {
            _childrenContainer[i] = _childrenContainer[i + 1];
        }
    }

    void EntityHierarchyManager::packFreeRanges()
    {
        // TODO: get the test.cpp thing here!
        if (_freeRanges.empty())
            return;

        std::map<size_t, size_t> result;
        std::map<size_t, size_t>::iterator currentIt = _freeRanges.begin();
        std::map<size_t, size_t>::iterator nextIt = currentIt;

        size_t currentOffset = currentIt->first;
        size_t currentCount = currentIt->second;
        size_t currentLastOffset = currentOffset + currentCount - 1;
        size_t nextIterIncr = 1;
        while (true)
        {
            ++nextIt;
            if (nextIt == _freeRanges.end())
                break;

            const size_t nextOffset = nextIt->first;
            const size_t nextCount = nextIt->second;
            // If next beginst right after current
            //  -> merge the next to the current
            if (currentLastOffset + 1 == nextOffset)
            {
                const size_t newCount = currentCount + nextCount;
                result[currentIt->first] = newCount;
                currentCount = newCount;
                currentLastOffset = currentOffset + currentCount - 1;
                ++nextIterIncr;
            }
            else
            {
                result[currentIt->first] = currentCount;
                result[nextIt->first] = nextCount;
                for (size_t i = 0; i < nextIterIncr; ++i)
                    ++currentIt;

                nextIterIncr = 1;
                currentOffset = currentIt->first;
                currentCount = currentIt->second;
                currentLastOffset = currentOffset + currentCount - 1;
            }
        }
        _freeRanges = result;
    }
}
