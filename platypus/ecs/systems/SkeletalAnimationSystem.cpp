#include "SkeletalAnimationSystem.hpp"
#include "platypus/ecs/components/Component.hpp"
#include "platypus/ecs/components/SkeletalAnimation.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"
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
            if (!shouldUpdate(entity))
                continue;

            void* pAnimationComponent = pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );
            PLATYPUS_ASSERT(pAnimationComponent);
            SkeletalAnimation* pAnimation = reinterpret_cast<SkeletalAnimation*>(pAnimationComponent);

            float& animationTime = pAnimation->time;
            if (animationTime < pAnimation->length)
                animationTime += 1.0f * Timing::get_delta_time();
            else if(pAnimation->mode == AnimationMode::ANIMATION_MODE_LOOP)
                animationTime = 0.0f;
        }
    }
}
