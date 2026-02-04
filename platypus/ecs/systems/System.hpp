#pragma once

#include <cstdint>
#include "platypus/ecs/Entity.hpp"


namespace platypus
{
    class Scene;

    class System
    {
    protected:
        uint64_t _requiredComponentMask = 0;

    public:
        System() = default;
        System(const System& other) = delete;
        virtual ~System() {}

        virtual void update(Scene* pScene) = 0;

        inline bool shouldUpdate(const Entity& entity)
        {
            return ((entity.componentMask & _requiredComponentMask) == _requiredComponentMask) && entity.active;
        }
    };
}
