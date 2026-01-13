#pragma once

#include "System.hpp"

namespace platypus
{
    class SkeletalAnimationSystem : public System
    {
    public:
        SkeletalAnimationSystem();
        ~SkeletalAnimationSystem();

        void update(Scene* pScene);
    };
}
