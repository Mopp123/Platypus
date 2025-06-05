#include "Transform.h"
#include "platypus/core/Application.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    Transform* create_transform(
        entityID_t target,
        Matrix4f matrix
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_TRANSFORM;
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
        Transform* pTransform = (Transform*)pComponent;
        pTransform->globalMatrix = matrix;

        return pTransform;
    }


    Transform* create_transform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale
    )
    {
        Matrix4f transformationMatrix = create_transformation_matrix(
            position,
            rotation,
            scale
        );

        return create_transform(target, transformationMatrix);
    }


    static void create_transform_entity_hierarchy(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        const Matrix4f& parentMatrix,
        int jointIndex,
        std::vector<entityID_t>& outEntities
    )
    {
        Matrix4f matrix = parentMatrix * joints[jointIndex].matrix;

        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        entityID_t entity = pScene->createEntity();
        outEntities.emplace_back(entity);
        Transform* pTransform = create_transform(
            entity,
            matrix
        );

        const Joint& currentJoint = joints[jointIndex];

        if (jointIndex != 0)
            pTransform->globalMatrix = parentMatrix * currentJoint.matrix;

        for (int childJointIndex : jointChildMapping[jointIndex])
        {
            create_transform_entity_hierarchy(
                joints,
                jointChildMapping,
                matrix,
                childJointIndex,
                outEntities
            );
        }
    }

    std::vector<entityID_t> create_skeleton(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping
    )
    {
        std::vector<entityID_t> entities;
        entities.reserve(joints.size());

        create_transform_entity_hierarchy(
            joints,
            jointChildMapping,
            Matrix4f(1.0f),
            0,
            entities
        );
        return entities;
    }


    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_gui_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_gui_transform "
                "Failed to allocate GUITransform component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        GUITransform* pTransform = (GUITransform*)pComponent;
        pTransform->position = position;
        pTransform->scale = scale;

        return pTransform;
    }
}
