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

    std::vector<char> serialize(const SkeletalAnimation* pSkeletalAnimation)
    {
        std::vector<char> serializedData(serialized_skeletal_animation_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletalAnimation->mode),
            sizeof(AnimationMode)
        );
        pos += sizeof(AnimationMode);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletalAnimation->animationID),
            sizeof(ID_t)
        );
        pos += sizeof(ID_t);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletalAnimation->time),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletalAnimation->length),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletalAnimation->stopped),
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        memcpy(
            serializedData.data() + pos,
            pSkeletalAnimation->jointMatrices,
            sizeof(Matrix4f) * skeletal_animation_max_joints
        );
        pos += sizeof(Matrix4f) * skeletal_animation_max_joints;

        return serializedData;
    }

    std::vector<char> serialize(const SkeletonJoint* pSkeletonJoint)
    {
        std::vector<char> serializedData(serialized_skeleton_joint_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_JOINT;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pSkeletonJoint->jointIndex),
            sizeof(uint32_t)
        );
        return serializedData;
    }
}
