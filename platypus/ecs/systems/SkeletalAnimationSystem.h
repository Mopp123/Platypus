#pragma once

#include "System.h"

namespace platypus
{
    class SkeletalAnimationSystem : public System
    {
    public:
        SkeletalAnimationSystem();
        SkeletalAnimationSystem(const SkeletalAnimationSystem& other);
        ~SkeletalAnimationSystem();

        void update(Scene* pScene);
    };
}
