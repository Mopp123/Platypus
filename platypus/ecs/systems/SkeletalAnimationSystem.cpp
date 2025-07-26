#include "SkeletalAnimationSystem.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/core/Application.h"
#include "platypus/core/Timing.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static void apply_interpolation_to_joints(
        Scene* pScene,
        entityID_t rootJointEntity,
        SkeletalAnimation* pAnimationComponent,
        const Pose& bindPose,
        const Pose& current,
        const Pose& next,
        float amount,
        uint32_t currentJointIndex,
        const Matrix4f& parentMatrix
    )
    {
        const Joint& jointCurrentPose = current.joints[currentJointIndex];
        const Joint& jointNextPose = next.joints[currentJointIndex];

        // TODO: Include scaling
        Vector3f interpolatedTranslation = jointCurrentPose.translation.lerp(jointNextPose.translation, amount);
        Matrix4f translationMatrix(1.0f);
        translationMatrix[0 + 3 * 4] = interpolatedTranslation.x;
        translationMatrix[1 + 3 * 4] = interpolatedTranslation.y;
        translationMatrix[2 + 3 * 4] = interpolatedTranslation.z;
        Quaternion interpolatedRotation = jointCurrentPose.rotation.slerp(jointNextPose.rotation, amount);

        const Matrix4f& inverseBindMatrix = bindPose.joints[currentJointIndex].inverseMatrix;
        Matrix4f m = parentMatrix * translationMatrix * interpolatedRotation.toRotationMatrix();
        Matrix4f resultMatrix = m * inverseBindMatrix; // Looks funky atm but when using inverse bind pose matrix in vertex shader only, we can make this so that it won't affect the transforms

        // NOTE: Just testing atm! DANGEROUS AS HELL!!!
        // *Allocated transform skeleton should be able to be accessed like this
        // TODO: Make this safe and faster
        size_t jointCount = bindPose.joints.size();
        Transform* pJointTransform = (Transform*)pScene->getComponent(
            rootJointEntity + currentJointIndex,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        pJointTransform->globalMatrix = resultMatrix;

        /*
        pAnimationComponent->resultPose.joints[currentJointIndex] = {
            interpolatedTranslation,
            interpolatedRotation,
            { 1.0f, 1.0f, 1.0f }, // Atm scaling is disabled!
            resultMatrix
        };
        */
        /*
        pAnimationComponent->setResultPoseJoint(
            {
                interpolatedTranslation,
                interpolatedRotation,
                { 1.0f, 1.0f, 1.0f }, // Atm scaling is disabled!
                resultMatrix
            },
            currentJointIndex
        );
        */

        const std::vector<uint32_t>& childJoints = bindPose.jointChildMapping[currentJointIndex];
        for (uint32_t i = 0; i < (uint32_t)childJoints.size(); ++i)
        {
            int childJointIndex = childJoints[i];
            apply_interpolation_to_joints(
                pScene,
                rootJointEntity,
                pAnimationComponent,
                bindPose,
                current,
                next,
                amount,
                childJointIndex,
                m
            );
        }
    }

    SkeletalAnimationSystem::SkeletalAnimationSystem()
    {}

    SkeletalAnimationSystem::SkeletalAnimationSystem(const SkeletalAnimationSystem& other)
    {}

    SkeletalAnimationSystem::~SkeletalAnimationSystem()
    {}

    void SkeletalAnimationSystem::update(Scene* pScene)
    {
        const uint64_t requiredComponentMask = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION | ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();

        for (const Entity& entity : pScene->getEntities())
        {
            if ((entity.componentMask & requiredComponentMask) != requiredComponentMask)
                continue;

            SkeletalAnimation* pAnimationComponent = (SkeletalAnimation*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );
            SkeletalAnimationData* pAnimationAsset = (SkeletalAnimationData*)pAssetManager->getAsset(
                pAnimationComponent->animationID,
                AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA
            );

            // TODO: Overly complicated below -> clean that up!!
            size_t poseCount = pAnimationAsset->getPoseCount();
            if (poseCount == 0)
            {
                Debug::log(
                    "@SkeletalAnimationSystem::update "
                    "Animation asset with ID: " + std::to_string(pAnimationAsset->getID()) + " "
                    "had 0 keyframes",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            const AnimationMode& animMode = pAnimationComponent->mode;

            uint32_t& currentPoseIndex = pAnimationComponent->currentPose;
            uint32_t& nextPoseIndex = pAnimationComponent->nextPose;

            Pose currentPose;
            Pose nextPose;

            currentPose = pAnimationAsset->getPose(currentPoseIndex);
            nextPose = pAnimationAsset->getPose(nextPoseIndex);

            apply_interpolation_to_joints(
                pScene,
                entity.id,
                pAnimationComponent,
                pAnimationAsset->getBindPose(),
                currentPose,
                nextPose,
                pAnimationComponent->progress,
                0,
                Matrix4f(1.0f)
            );

            pAnimationComponent->progress += pAnimationComponent->speed * Timing::get_delta_time();
            if (pAnimationComponent->progress >= 1.0f)
            {
                pAnimationComponent->progress = 0.0f;

                currentPoseIndex += 1;
                nextPoseIndex += 1;

                // TODO: Make sure "play once" anims work
                if (nextPoseIndex >= poseCount && animMode == AnimationMode::ANIMATION_MODE_PLAY_ONCE)
                    pAnimationComponent->stopped = true;

                currentPoseIndex %= poseCount;
                nextPoseIndex %= poseCount;
            }
        }
    }
}
