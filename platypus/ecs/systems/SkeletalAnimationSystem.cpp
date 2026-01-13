#include "SkeletalAnimationSystem.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Timing.hpp"


namespace platypus
{
    SkeletalAnimationSystem::SkeletalAnimationSystem()
    {
        _requiredComponentMask = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
    }

    SkeletalAnimationSystem::~SkeletalAnimationSystem()
    {}

    void SkeletalAnimationSystem::update(Scene* pScene)
    {
        for (const Entity& entity : pScene->getEntities())
        {
            if ((entity.componentMask & _requiredComponentMask) != _requiredComponentMask)
                continue;

            SkeletalAnimation* pAnimationComponent = (SkeletalAnimation*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );

            float& animationTime = pAnimationComponent->time;
            if (animationTime < pAnimationComponent->length)
                animationTime += 1.0f * Timing::get_delta_time();
            else if(pAnimationComponent->mode == AnimationMode::ANIMATION_MODE_LOOP)
                animationTime = 0.0f;
        }
    }
}
