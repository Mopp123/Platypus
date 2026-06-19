#include "Entity.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"
#include <cstring>


namespace platypus
{
    size_t get_component_count(uint64_t componentMask)
    {
        size_t count = 0;
        for (size_t i = 0; i < sizeof(uint64_t); ++i)
        {

            if (componentMask & 0x1 << i)
                ++count;
        }
        return count;
    }


    std::string entity_error_to_string(EntityError error)
    {
        switch (error)
        {
            case EntityError::NO_ERROR: return "No error";
            case EntityError::COMPONENT_RENDERABLE3D_INCOMPATIBLE_MESH_MATERIAL: return "Renderable3D's Mesh and Material assets are incompatible";
        }
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
}
