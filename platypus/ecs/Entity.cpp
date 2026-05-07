#include "Entity.hpp"
#include <cstring>


namespace platypus
{
    Entity::Entity()
    {}

    Entity::Entity(const Entity& other) :
        id(other.id),
        componentMask(other.componentMask),
        active(other.active)
    {}

    void Entity::clear()
    {
        id = NULL_ENTITY_ID;
        componentMask = 0;
    }

    std::vector<char> serialize(const Entity& entity)
    {
        std::vector<char> serializedData(serialized_entity_size);
        memcpy(
            serializedData.data(),
            &entity.id,
            sizeof(entityID_t)
        );
        size_t pos = sizeof(entityID_t);

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

        return serializedData;
    }
}
