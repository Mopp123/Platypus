#include "TransformSystem.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Application.h"
#include "platypus/assets/AssetManager.h"
#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    // ISSUES(?):
    //      * Root joint animation issue
    //          - If animated skeleton's root joint doesn't have a parent,
    //          it doesn't get animated.
    //          SOLUTION:
    //              -> Always have some "base entity" that has the root joint entity as child
    static void apply_transform_hierarchy(
        Scene* pScene,
        AssetManager* pAssetManager,
        entityID_t entity,
        entityID_t parent,
        SkeletalAnimationData* pAnimationAsset,
        SkeletalAnimation* pUseAnimation,
        const Pose* pBindPose,
        Pose* pCurrentPose,
        Pose* pNextPose
    )
    {
        // Attempt to find bind pose if exists
        // NOTE: Kind of rough to check for each possible joint...
        Renderable3D* pRenderable = (Renderable3D*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_RENDERABLE3D,
            false,
            false
        );
        if (pRenderable)
        {
            Mesh* pMesh = (Mesh*)pAssetManager->getAsset(
                pRenderable->meshID,
                AssetType::ASSET_TYPE_MESH
            );
            if (pMesh->getType() == MeshType::MESH_TYPE_SKINNED)
                pBindPose = pMesh->getBindPosePtr();
        }


        // Check if animation changes for this entity and its children
        //  -> need to find new animation asset
        SkeletalAnimation* pEntityAnimation = (SkeletalAnimation*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION,
            false,
            false
        );
        if (pEntityAnimation && (pEntityAnimation != pUseAnimation))
        {
            ID_t prevAnimAssetID = NULL_ID;
            if (pUseAnimation)
                prevAnimAssetID = pUseAnimation->animationID;
            pUseAnimation = pEntityAnimation;
            if (prevAnimAssetID != pUseAnimation->animationID)
            {
                pAnimationAsset = (SkeletalAnimationData*)pAssetManager->getAsset(
                    pUseAnimation->animationID,
                    AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA
                );
            }
        }

        Transform* pTransform = (Transform*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        if (!pTransform)
        {
            Debug::log(
                "@apply_transform_hierarchy "
                "Entity " + std::to_string(entity) + " doesn't have a Transform component!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // Apply animation if exists
        Matrix4f localMatrix = pTransform->localMatrix;
        SkeletonJoint* pJoint = nullptr;
        if (pUseAnimation && pAnimationAsset)
        {
            pJoint = (SkeletonJoint*)pScene->getComponent(
                entity,
                ComponentType::COMPONENT_TYPE_JOINT,
                false,
                false
            );
            if (pJoint)
            {
                size_t jointIndex = pJoint->jointIndex;
                Matrix4f animatedBoneMatrix = pAnimationAsset->getBoneMatrix(
                    pUseAnimation->time,
                    jointIndex
                );
                localMatrix = animatedBoneMatrix;
            }
            // Went outside the bounds of prev anim joints -> reset anim
            else
            {
                pUseAnimation = nullptr;
                pAnimationAsset = nullptr;
            }
        }

        // Apply parent transform if exists
        Matrix4f parentMatrix(1.0f);
        if (parent != NULL_ENTITY_ID)
        {
            Transform* pParentTransform = (Transform*)pScene->getComponent(
                parent,
                ComponentType::COMPONENT_TYPE_TRANSFORM
            );
            parentMatrix = pParentTransform->globalMatrix;
            pTransform->globalMatrix = parentMatrix * localMatrix;
        }

        if (pJoint && pUseAnimation && pBindPose)
        {
            pUseAnimation->jointMatrices[pJoint->jointIndex] = pTransform->globalMatrix * pBindPose->joints[pJoint->jointIndex].inverseMatrix;
        }

        Children* pChildren = (Children*)pScene->getComponent(
            entity,
            ComponentType::COMPONENT_TYPE_CHILDREN,
            false,
            false
        );
        if (!pChildren)
            return;

        for (size_t childIndex = 0; childIndex < pChildren->count; ++childIndex)
        {
            entityID_t childEntity = pChildren->entityIDs[childIndex];
            apply_transform_hierarchy(
                pScene,
                pAssetManager,
                childEntity,
                entity,
                pAnimationAsset,
                pUseAnimation,
                pBindPose,
                pCurrentPose,
                pNextPose
            );
        }
    }


    // NOTE: ISSUE!
    //  Not allowing animating single bone skeletons
    //  -> skipping if no Children componen found!
    //  TODO: Fix this when animating here!
    TransformSystem::TransformSystem()
    {
        _requiredComponentMask = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_CHILDREN;
    }

    TransformSystem::~TransformSystem()
    {
    }

    void TransformSystem::update(Scene* pScene)
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (const Entity& entity : pScene->getEntities())
        {
            // Handle only root entities
            // -> their children are handled by the apply_transform_hierarchy func
            bool hasRequiredComponentMask = (entity.componentMask & _requiredComponentMask) == _requiredComponentMask;
            bool isRoot = (entity.componentMask & ComponentType::COMPONENT_TYPE_PARENT) == 0;
            if (!hasRequiredComponentMask || !isRoot)
            {
                continue;
            }
            entityID_t entityID = entity.id;

            SkeletalAnimation* pAnimationComponent = (SkeletalAnimation*)pScene->getComponent(
                entityID,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION,
                false,
                false
            );
            SkeletalAnimationData* pAnimationAsset = nullptr;
            if (pAnimationComponent)
            {
                pAnimationAsset = (SkeletalAnimationData*)pAssetManager->getAsset(
                    pAnimationComponent->animationID,
                    AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA
                );
            }

            apply_transform_hierarchy(
                pScene,
                pAssetManager,
                entityID,
                NULL_ENTITY_ID,
                pAnimationAsset,
                pAnimationComponent,
                nullptr,
                nullptr,
                nullptr
            );
        }
    }
}
