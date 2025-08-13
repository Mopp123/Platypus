#pragma once

#include "System.h"

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
