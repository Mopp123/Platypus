#include "SkeletalAnimation.h"
#include "platypus/assets/SkeletalAnimationData.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Scene.h"


namespace platypus
{
    SkeletalAnimation* create_skeletal_animation(
        entityID_t target,
        ID_t animationAssetID,
        float speed
    )
    {
        Application* pApp = Application::get_instance();
        Scene* pScene = pApp->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_skeletal_animation"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_transform(2) "
                "Failed to allocate Transform component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);

        SkeletalAnimation* pAnimation = (SkeletalAnimation*)pComponent;
        pAnimation->mode = AnimationMode::ANIMATION_MODE_LOOP;
        pAnimation->animationID = animationAssetID;
        pAnimation->currentPose = 0;
        pAnimation->nextPose = 1;
        pAnimation->progress = 0.0f;
        pAnimation->speed = speed;
        pAnimation->stopped = false;

        return pAnimation;
    }
}
