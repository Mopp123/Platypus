#pragma once

#include "platypus/utils/UUID.hpp"
#include <vector>
#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    class Scene;

    constexpr size_t serialized_entity_size =
        sizeof(UUID_t) +
        sizeof(uint64_t) +
        sizeof(uint8_t);

    constexpr size_t serialized_entities_header_size = sizeof(uint32_t);
    constexpr uint32_t entity_uuid_pool_id = 1;

    struct Entity
    {
        entityID_t id = NULL_ENTITY_ID;
        UUID_t uuid = NULL_UUID;

        uint64_t componentMask = 0;
        uint8_t active = 1;

        Entity();
        Entity(const Entity& other);
        void clear();
    };

    size_t get_component_count(uint64_t componentMask);

    // NOTE:
    // *entityID_t not included in serialized format, ONLY THE UUID!
    // *Should we rather return the UUID instead of the entityID_t here?
    std::vector<char> serialize_entity(const Entity& entity);
    void deserialize_entity(
        Scene* pScene,
        Entity& outEntity,
        size_t dataSize,
        const void* pData
    );
}
