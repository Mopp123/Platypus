#include "SkeletalAnimation.hpp"
#include "platypus/assets/SkeletalAnimationData.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    SkeletalAnimation* create_skeletal_animation(
        entityID_t target,
        ID_t animationAssetID,
        Scene* pScene
    )
    {
        Application* pApp = Application::get_instance();
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = pApp->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_skeletal_animation"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_skeletal_animation "
                "Failed to allocate SkeletalAnimation component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pUseScene->addToComponentMask(target, componentType);

        SkeletalAnimationData* pAsset = (SkeletalAnimationData*)pApp->getAssetManager()->getAsset(
            animationAssetID,
            AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA
        );

        SkeletalAnimation* pAnimation = (SkeletalAnimation*)pComponent;
        pAnimation->mode = AnimationMode::ANIMATION_MODE_LOOP;
        pAnimation->animationID = animationAssetID;
        pAnimation->time = 0.0f;
        pAnimation->length = pAsset->getLength();
        pAnimation->stopped = false;

        return pAnimation;
    }


    SkeletonJoint* create_skeleton_joint(
        entityID_t target,
        uint32_t jointIndex,
        Scene* pScene
    )
    {
        Application* pApp = Application::get_instance();
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = pApp->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_skeleton_joint"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_JOINT;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_skeleton_joint "
                "Failed to allocate Joint component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pUseScene->addToComponentMask(target, componentType);

        SkeletonJoint* pJoint = (SkeletonJoint*)pComponent;
        pJoint->jointIndex = jointIndex;

        return pJoint;
    }
}
