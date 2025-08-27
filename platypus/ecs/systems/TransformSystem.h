#pragma once

#include "System.h"

namespace platypus
{
    class TransformSystem : public System
    {
    public:
        TransformSystem();
        TransformSystem(const TransformSystem& other) = delete;
        ~TransformSystem();

        virtual void update(Scene* pScene);
    };
}
