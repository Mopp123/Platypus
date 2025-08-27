#pragma once

#include <cstdint>

#define NULL_ENTITY_ID -1
typedef int64_t entityID_t;


namespace platypus
{
    struct Entity
    {
        entityID_t id = NULL_ENTITY_ID;
        // TODO: remove parent from here -> that kind of stuff handled using Parent and Children components!
        entityID_t parentID = NULL_ENTITY_ID;
        uint64_t componentMask = 0;

        Entity() {}
        Entity(const Entity& other) :
            id(other.id),
            parentID(other.parentID),
            componentMask(other.componentMask)
        {}

        void clear()
        {
            id = NULL_ENTITY_ID;
            parentID = NULL_ENTITY_ID;
            componentMask = 0;
        }
    };
}
