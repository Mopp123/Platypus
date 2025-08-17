#include "TransformSystem.h"
#include "platypus/core/Scene.h"
#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static void apply_transform_hierarchy(
        Scene* pScene,
        entityID_t entity,
        entityID_t parent
    )
    {
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

        if (parent != NULL_ENTITY_ID)
        {
            Transform* pParentTransform = (Transform*)pScene->getComponent(
                parent,
                ComponentType::COMPONENT_TYPE_TRANSFORM
            );
            pTransform->globalMatrix = pParentTransform->globalMatrix * pTransform->localMatrix;
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
                childEntity,
                entity
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
            apply_transform_hierarchy(
                pScene,
                entityID,
                NULL_ENTITY_ID
            );
        }
    }
}
