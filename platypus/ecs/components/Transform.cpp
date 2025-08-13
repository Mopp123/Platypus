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


    void set_transform_position(Transform* pTransform, const Vector3f& position, bool hasParent)
    {
        Matrix4f& m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        m[0 + 3 * 4] = position.x;
        m[1 + 3 * 4] = position.y;
        m[2 + 3 * 4] = position.z;
    }

    void set_transform_rotation(Transform* pTransform, float pitch, float yaw, float roll, bool hasParent)
    {
        Matrix4f& m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        Matrix4f rotationMatrix = create_rotation_matrix(pitch, yaw, roll);
	    m[1 + 1 * 4] = rotationMatrix[1 + 1 * 4];
	    m[1 + 2 * 4] = rotationMatrix[1 + 2 * 4];
	    m[2 + 1 * 4] = rotationMatrix[2 + 1 * 4];
	    m[2 + 2 * 4] = rotationMatrix[2 + 2 * 4];

	    m[0 + 0 * 4] = rotationMatrix[0 + 0 * 4];
	    m[0 + 2 * 4] = rotationMatrix[0 + 2 * 4];
	    m[2 + 0 * 4] = rotationMatrix[2 + 0 * 4];
	    m[2 + 2 * 4] = rotationMatrix[2 + 2 * 4];

	    m[0 + 0 * 4] = rotationMatrix[0 + 0 * 4];
	    m[0 + 1 * 4] = rotationMatrix[0 + 1 * 4];
	    m[1 + 0 * 4] = rotationMatrix[1 + 0 * 4];
	    m[1 + 1 * 4] = rotationMatrix[1 + 1 * 4];
    }

    void set_transform_rotation(Transform* pTransform, const Quaternion& rotation, bool hasParent)
    {
        Matrix4f& m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        Matrix4f rotationMatrix = rotation.toRotationMatrix();
	    m[1 + 1 * 4] = rotationMatrix[1 + 1 * 4];
	    m[1 + 2 * 4] = rotationMatrix[1 + 2 * 4];
	    m[2 + 1 * 4] = rotationMatrix[2 + 1 * 4];
	    m[2 + 2 * 4] = rotationMatrix[2 + 2 * 4];

	    m[0 + 0 * 4] = rotationMatrix[0 + 0 * 4];
	    m[0 + 2 * 4] = rotationMatrix[0 + 2 * 4];
	    m[2 + 0 * 4] = rotationMatrix[2 + 0 * 4];
	    m[2 + 2 * 4] = rotationMatrix[2 + 2 * 4];

	    m[0 + 0 * 4] = rotationMatrix[0 + 0 * 4];
	    m[0 + 1 * 4] = rotationMatrix[0 + 1 * 4];
	    m[1 + 0 * 4] = rotationMatrix[1 + 0 * 4];
	    m[1 + 1 * 4] = rotationMatrix[1 + 1 * 4];
    }

    void rotate_transform(Transform* pTransform, float pAmount, float yAmount, float rAmount, bool hasParent)
    {
        Matrix4f& m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        m = m * create_rotation_matrix(pAmount, yAmount, rAmount);
    }

    void set_transform_scale(Transform* pTransform, const Vector3f& scale, bool hasParent)
    {
        // NOTE: This might be incorrect!!!
        Matrix4f& m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;

        Matrix4f scaleMatrix(1.0f);
        scaleMatrix[0 + 0 * 4] = scale.x;
        scaleMatrix[1 + 1 * 4] = scale.y;
        scaleMatrix[2 + 2 * 4] = scale.z;

        float currentSX = Vector3f(m[0 + 0 * 4], m[1 + 0 * 4], m[2 + 0 * 4]).length();
        float currentSY = Vector3f(m[0 + 1 * 4], m[1 + 1 * 4], m[2 + 1 * 4]).length();
        float currentSZ = Vector3f(m[0 + 2 * 4], m[1 + 2 * 4], m[2 + 2 * 4]).length();

        m[0 + 0 * 4] = m[0 + 0 * 4] / currentSX * scale.x;
        m[1 + 1 * 4] = m[1 + 1 * 4] / currentSY * scale.y;
        m[2 + 2 * 4] = m[2 + 2 * 4] / currentSZ * scale.z;
    }

    Vector3f get_transform_forward(Transform* pTransform)
    {
        Matrix4f& m = pTransform->globalMatrix;
        Vector3f backwards(
                m[0 + 2 * 4],
                m[1 + 2 * 4],
                m[2 + 2 * 4]
                );
        return backwards * -1.0f;
    }

    Vector3f get_transform_up(Transform* pTransform)
    {
        Matrix4f& m = pTransform->globalMatrix;
        return {
            m[0 + 1 * 4],
            m[1 + 1 * 4],
            m[2 + 1 * 4]
        };
    }

    Vector3f get_transform_right(Transform* pTransform)
    {
        Matrix4f& m = pTransform->globalMatrix;
        return {
            m[0 + 0 * 4],
            m[1 + 0 * 4],
            m[2 + 0 * 4]
        };
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


    void add_child(entityID_t target, entityID_t child)
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "add_child"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }

        // Check if target already have Children component
        Children* pChildren = (Children*)pScene->getComponent(
            target,
            ComponentType::COMPONENT_TYPE_CHILDREN,
            false,
            false
        );
        if (!pChildren)
        {
            void* pChildrenComponent = pScene->allocateComponent(
                target,
                ComponentType::COMPONENT_TYPE_CHILDREN
            );
            if (!pChildrenComponent)
            {
                Debug::log(
                    "@add_child "
                    "Failed to allocate Children component for entity: " + std::to_string(target),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            pScene->addToComponentMask(target, ComponentType::COMPONENT_TYPE_CHILDREN);
            pChildren = (Children*)pChildrenComponent;
            pChildren->count = 0;
            memset(pChildren, 0, sizeof(Children));
        }
        // Make sure child count within limits
        if (pChildren->count >= PLATYPUS_MAX_CHILD_ENTITIES)
        {
            Debug::log(
                "@add_child "
                "Child count exceeded for entity: " + std::to_string(target) + " "
                "Max child count is: " + std::to_string(PLATYPUS_MAX_CHILD_ENTITIES),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // Add the child entity to Children and create Parent component for the child
        pChildren->entityIDs[pChildren->count] = child;
        ++pChildren->count;

        void* pParentComponent = pScene->allocateComponent(
            target,
            ComponentType::COMPONENT_TYPE_PARENT
        );
        if (!pParentComponent)
        {
            Debug::log(
                "@add_child "
                "Failed to allocate Parent component for entity: " + std::to_string(child),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        pScene->addToComponentMask(child, ComponentType::COMPONENT_TYPE_PARENT);
        Parent* pParent = (Parent*)pParentComponent;
        pParent->entityID = target;

        // If child entity has transform, switch it's global matrix to be local
        Transform* pChildTransform = (Transform*)pScene->getComponent(
            child,
            ComponentType::COMPONENT_TYPE_TRANSFORM,
            false,
            false
        );
        if (pChildTransform)
        {
            pChildTransform->localMatrix = pChildTransform->globalMatrix;
            pChildTransform->globalMatrix = Matrix4f(1.0f);
        }
        Debug::log("___TEST___Added child entity: " + std::to_string(child) + " to parent entity: " + std::to_string(target));
    }
}
