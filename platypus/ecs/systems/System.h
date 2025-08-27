#pragma once

#include <cstdint>


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
    };
}
