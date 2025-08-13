#include "TransformSystem.h"
#include "platypus/core/Scene.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static void apply_transform_hierarchy(
        Scene* pScene,
        entityID_t parent,
        Children* pChildren
    )
    {
        Transform* pParentTransform = (Transform*)pScene->getComponent(
            parent,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        for (size_t i = 0; i < pChildren->count; ++i)
        {
            entityID_t childEntity = pChildren->entityIDs[i];
            Transform* pChildTransform = (Transform*)pScene->getComponent(
                childEntity,
                ComponentType::COMPONENT_TYPE_TRANSFORM
            );
            if (pChildTransform)
            {
                pChildTransform->globalMatrix = pParentTransform->globalMatrix * pChildTransform->localMatrix;
                Children* pChildChildren = (Children*)pScene->getComponent(
                    childEntity,
                    ComponentType::COMPONENT_TYPE_CHILDREN,
                    false,
                    false
                );
                if (pChildChildren)
                    apply_transform_hierarchy(pScene, childEntity, pChildChildren);
            }
        }
    }


    // NOTE: CURRENT ISSUE!
    // Atm not allowing animating single bone skeletons
    //  -> skipping of no Children componen found!
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
            Children* pChildren = (Children*)pScene->getComponent(
                entityID,
                ComponentType::COMPONENT_TYPE_CHILDREN
            );
            apply_transform_hierarchy(
                pScene,
                entityID,
                pChildren
            );
        }
    }
}
