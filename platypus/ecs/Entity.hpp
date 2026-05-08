#pragma once

#include <vector>
#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    class Scene;

    constexpr size_t serialized_entity_size =
        sizeof(entityID_t) +
        sizeof(uint64_t) +
        sizeof(uint8_t);

    struct Entity
    {
        entityID_t id = NULL_ENTITY_ID;
        uint64_t componentMask = 0;
        uint8_t active = 1;

        Entity();
        Entity(const Entity& other);
        void clear();
    };

    size_t get_component_count(uint64_t componentMask);

    std::vector<char> serialize_entity(const Entity& entity);
    void deserialize_entity(
        Scene* pScene,
        entityID_t& outEntity,
        size_t dataSize,
        void* pData
    );
}
