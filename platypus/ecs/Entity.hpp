#pragma once

#include <vector>
#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    constexpr size_t serialized_entity_size =
        sizeof(entityID_t) +
        sizeof(uint64_t) +
        sizeof(uint8_t);

    struct Entity
    {
        entityID_t id = NULL_ENTITY_ID;
        // TODO: remove parent from here -> that kind of stuff handled using Parent and Children components!
        uint64_t componentMask = 0;
        uint8_t active = 1;

        Entity();
        Entity(const Entity& other);
        void clear();
    };

    std::vector<char> serialize(const Entity& entity);
}
