#include "Transform.hpp"
#include "platypus/core/Application.hpp"
#include "SkeletalAnimation.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    Transform* create_transform(
        entityID_t target,
        Matrix4f matrix,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_TRANSFORM;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        Transform* pTransform = (Transform*)pComponent;
        pTransform->globalMatrix = matrix;

        return pTransform;
    }


    Transform* create_transform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Matrix4f transformationMatrix = create_transformation_matrix(
            position,
            rotation,
            scale
        );

        return create_transform(target, transformationMatrix, pScene, useExplicitComponentMask);
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

    Vector3f get_transform_position(const Transform* pTransform, bool hasParent)
    {
        const Matrix4f m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        return {
            m[0 + 3 * 4],
            m[1 + 3 * 4],
            m[2 + 3 * 4]
        };
    }

    // *Got this and get_transform_scale(...) from: https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati
    // NOTE: This uses ZYX order! NOT TESTED, IS PROBABLY FUCKED!
    Vector3f get_transform_euler_rotation(const Transform* pTransform, bool hasParent)
    {
        const Matrix4f m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        float sx = Vector3f(m[0 + 0 * 4], m[1 + 0 * 4], m[2 + 0 * 4]).length();
        float sy = Vector3f(m[0 + 1 * 4], m[1 + 1 * 4], m[2 + 1 * 4]).length();
        float sz = Vector3f(m[0 + 2 * 4], m[1 + 2 * 4], m[2 + 2 * 4]).length();

        Matrix4f rotationMatrix(1.0f);
        rotationMatrix[0 + 0 * 4] = m[0 + 0 * 4] / sx;
        rotationMatrix[1 + 0 * 4] = m[1 + 0 * 4] / sx;
        rotationMatrix[2 + 0 * 4] = m[2 + 0 * 4] / sx;

        rotationMatrix[0 + 1 * 4] = m[0 + 1 * 4] / sy;
        rotationMatrix[1 + 1 * 4] = m[1 + 1 * 4] / sy;
        rotationMatrix[2 + 1 * 4] = m[2 + 1 * 4] / sy;

        rotationMatrix[0 + 2 * 4] = m[0 + 2 * 4] / sz;
        rotationMatrix[1 + 2 * 4] = m[1 + 2 * 4] / sz;
        rotationMatrix[2 + 2 * 4] = m[2 + 2 * 4] / sz;

        float r00 = rotationMatrix[0];
        float r10 = rotationMatrix[1 + 0 * 4];
        float pitch = std::atan2(
            -rotationMatrix[2 + 0 * 4],
            std::sqrt(r00 * r00 + r10 * r10)
        );

        float yaw = 0.0f;
        float roll = 0.0f;

        // Solve Gimbal Lock
        if (std::cos(pitch) == 0.0f)
        {
            roll = 0.0f;
            yaw = std::atan2(rotationMatrix[0 + 1 * 4], rotationMatrix[1 + 1 * 4]);
        }
        else
        {
            yaw = std::atan2(r10, r00);
            roll = std::atan2(rotationMatrix[2 + 1 * 4], rotationMatrix[2 + 2 * 4]);
        }
        return { pitch, yaw, roll };
    }

    Vector3f get_transform_scale(const Transform* pTransform, bool hasParent)
    {
        const Matrix4f m = hasParent ? pTransform->localMatrix : pTransform->globalMatrix;
        float sx = Vector3f(m[0 + 0 * 4], m[1 + 0 * 4], m[2 + 0 * 4]).length();
        float sy = Vector3f(m[0 + 1 * 4], m[1 + 1 * 4], m[2 + 1 * 4]).length();
        float sz = Vector3f(m[0 + 2 * 4], m[1 + 2 * 4], m[2 + 2 * 4]).length();
        return { sx, sy, sz };
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

    Matrix4f get_global_rotation_matrix(const Transform* pTransform)
    {
        const Matrix4f& transformationMatrix = pTransform->globalMatrix;
        // NOTE: Not sure if rotationMatrix[3 + 3 * 4] should be 0 instead of 1 here?
        Matrix4f rotationMatrix(1.0f);
	    rotationMatrix[1 + 1 * 4] = transformationMatrix[1 + 1 * 4];
	    rotationMatrix[1 + 2 * 4] = transformationMatrix[1 + 2 * 4];
	    rotationMatrix[2 + 1 * 4] = transformationMatrix[2 + 1 * 4];
	    rotationMatrix[2 + 2 * 4] = transformationMatrix[2 + 2 * 4];

	    rotationMatrix[0 + 0 * 4] = transformationMatrix[0 + 0 * 4];
	    rotationMatrix[0 + 2 * 4] = transformationMatrix[0 + 2 * 4];
	    rotationMatrix[2 + 0 * 4] = transformationMatrix[2 + 0 * 4];
	    rotationMatrix[2 + 2 * 4] = transformationMatrix[2 + 2 * 4];

	    rotationMatrix[0 + 0 * 4] = transformationMatrix[0 + 0 * 4];
	    rotationMatrix[0 + 1 * 4] = transformationMatrix[0 + 1 * 4];
	    rotationMatrix[1 + 0 * 4] = transformationMatrix[1 + 0 * 4];
	    rotationMatrix[1 + 1 * 4] = transformationMatrix[1 + 1 * 4];

        return rotationMatrix;
    }

    static entityID_t create_transform_entity_hierarchy(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        const Matrix4f& parentMatrix,
        int jointIndex,
        std::vector<entityID_t>& outEntities,
        Scene* pScene
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        Matrix4f matrix = parentMatrix * joints[jointIndex].matrix;
        entityID_t entity = pUseScene->createEntity();
        outEntities.push_back(entity);
        Transform* pTransform = create_transform(
            entity,
            matrix
        );
        create_skeleton_joint(entity, jointIndex);

        const Joint& currentJoint = joints[jointIndex];

        if (jointIndex != 0)
            pTransform->globalMatrix = parentMatrix * currentJoint.matrix;

        for (int childJointIndex : jointChildMapping[jointIndex])
        {
            entityID_t childEntity = create_transform_entity_hierarchy(
                joints,
                jointChildMapping,
                matrix,
                childJointIndex,
                outEntities,
                pUseScene
            );
            add_child(entity, childEntity);
        }
        return entity;
    }

    std::vector<entityID_t> create_skeleton(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        Scene* pScene
    )
    {
        std::vector<entityID_t> jointEntities;
        create_transform_entity_hierarchy(
            joints,
            jointChildMapping,
            Matrix4f(1.0f),
            0,
            jointEntities,
            pScene
        );
        return jointEntities;
    }


    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_gui_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        GUITransform* pTransform = (GUITransform*)pComponent;
        pTransform->position = position;
        pTransform->scale = scale;

        return pTransform;
    }


    Parent* create_parent(
        entityID_t target,
        entityID_t parentID,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_parent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_PARENT;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Parent component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        Parent* pParent = reinterpret_cast<Parent*>(pComponent);
        pParent->entityID = parentID;
        return pParent;
    }


    Children* create_children(
        entityID_t target,
        std::vector<entityID_t> childIDs,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        PLATYPUS_ASSERT(childIDs.size() <= PLATYPUS_MAX_CHILD_ENTITIES);
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_children"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CHILDREN;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Children component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        Children* pChildren = reinterpret_cast<Children*>(pComponent);
        pChildren->count = childIDs.size();
        for (size_t i = 0; i < childIDs.size(); ++i)
            pChildren->entityIDs[i] = childIDs[i];

        return pChildren;
    }


    void add_child(entityID_t target, entityID_t child, Scene* pScene)
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "add_child"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }

        // Check if target already have Children component
        Children* pChildren = (Children*)pUseScene->getComponent(
            target,
            ComponentType::COMPONENT_TYPE_CHILDREN,
            false,
            false
        );
        if (!pChildren)
        {
            void* pChildrenComponent = pUseScene->allocateComponent(
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
            pUseScene->addToComponentMask(target, ComponentType::COMPONENT_TYPE_CHILDREN);
            pChildren = (Children*)pChildrenComponent;
            pChildren->count = 0;
            memset((void*)pChildren, 0, sizeof(Children));
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

        void* pParentComponent = pUseScene->allocateComponent(
            child,
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
        pUseScene->addToComponentMask(child, ComponentType::COMPONENT_TYPE_PARENT);
        Parent* pParent = (Parent*)pParentComponent;
        pParent->entityID = target;

        // If child entity has transform, switch it's global matrix to be local
        Transform* pChildTransform = (Transform*)pUseScene->getComponent(
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
    }


    void remove_child(entityID_t target, entityID_t child, Scene* pScene)
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        pUseScene->destroyComponent(child, ComponentType::COMPONENT_TYPE_PARENT);
        Children* pChildren = (Children*)pUseScene->getComponent(
            target,
            ComponentType::COMPONENT_TYPE_CHILDREN
        );
        for (size_t i = 0; i < pChildren->count; ++i)
        {
            if (pChildren->entityIDs[i] == child)
            {
                pChildren->entityIDs[i] = NULL_ENTITY_ID;
                pChildren->count -= 1;
                if (pChildren->count == 0)
                    pUseScene->destroyComponent(target, ComponentType::COMPONENT_TYPE_CHILDREN);

                pack_children(pChildren, i);
                return;
            }
        }
        Debug::log(
            "No child entity found with ID: " + std::to_string(child) + " "
            "from parent entity: " + std::to_string(target),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }


    void pack_children(Children* pChildren, size_t freedPosition)
    {
        // This should never happen since this gets called ONLY when children are removed
        //  -> I don't trust myself tho...
        if (pChildren->count >= PLATYPUS_MAX_CHILD_ENTITIES)
        {
            Debug::log(
                "Child count (" + std::to_string(pChildren->count) + ") "
                "exceeded maximum limit (" + std::to_string(PLATYPUS_MAX_CHILD_ENTITIES) + ") "
                "THIS SHOULD NEVER HAPPEN WHEN CALLING THIS FUCNTION! YOU'VE DONE FUCKED UP!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        std::vector<entityID_t> temp(pChildren->count);
        // We're moving all IDs one slot back from freedPosition
        // count + 1 since the last removal already decreased it
        for (size_t i = freedPosition + 1; i < pChildren->count + 1; ++i)
        {
            entityID_t childID = pChildren->entityIDs[i];
            pChildren->entityIDs[i - 1] = childID;
        }
        // Make last pos NULL_ENTITY since otherwise the last id gets duplicated
        pChildren->entityIDs[pChildren->count] = NULL_ENTITY_ID;
    }


    std::vector<char> serialize(const Transform* pTransform)
    {
        std::vector<char> serializedData(serialized_transform_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_TRANSFORM;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pTransform->localMatrix),
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            serializedData.data() + pos,
            &(pTransform->globalMatrix),
            sizeof(Matrix4f)
        );

        return serializedData;
    }


    std::vector<char> serialize(const GUITransform* pTransform)
    {
        std::vector<char> serializedData(serialized_gui_transform_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pTransform->position),
            sizeof(Vector2f)
        );
        pos += sizeof(Vector2f);

        memcpy(
            serializedData.data() + pos,
            &(pTransform->scale),
            sizeof(Vector2f)
        );

        return serializedData;
    }


    std::vector<char> serialize(const Parent* pParent)
    {
        std::vector<char> serializedData(serialized_parent_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_PARENT;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        const Scene* pScene = Application::get_instance()->getSceneManager().getCurrentScene();
        const Entity parentEntity = pScene->getEntity(pParent->entityID);
        PLATYPUS_ASSERT(parentEntity.id != NULL_ENTITY_ID);
        const UUID_t parentUUID = pScene->getEntity(pParent->entityID).uuid;
        PLATYPUS_ASSERT(parentUUID != NULL_UUID);
        memcpy(
            serializedData.data() + pos,
            &parentUUID,
            sizeof(UUID_t)
        );

        return serializedData;
    }


    std::vector<char> serialize(const Children* pChildren)
    {
        std::vector<char> serializedData(serialized_children_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_CHILDREN;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        const size_t childCount = pChildren->count;
        memcpy(
            serializedData.data() + pos,
            &(pChildren->count),
            sizeof(size_t)
        );
        pos += sizeof(size_t);

        const Scene* pScene = Application::get_instance()->getSceneManager().getCurrentScene();
        UUID_t childUUIDs[PLATYPUS_MAX_CHILD_ENTITIES];
        memset(childUUIDs, NULL_UUID, sizeof(UUID_t) * PLATYPUS_MAX_CHILD_ENTITIES);
        for (size_t i = 0; i < childCount; ++i)
        {
            const Entity childEntity = pScene->getEntity(pChildren->entityIDs[i]);
            PLATYPUS_ASSERT(childEntity.id != NULL_ENTITY_ID);
            const UUID_t childUUID = childEntity.uuid;
            PLATYPUS_ASSERT(childUUID != NULL_UUID);
            childUUIDs[i] = childUUID;
        }

        memcpy(
            serializedData.data() + pos,
            childUUIDs,
            sizeof(UUID_t) * PLATYPUS_MAX_CHILD_ENTITIES
        );

        return serializedData;
    }


    void deserialize(
        Scene* pScene,
        Transform** ppTransform,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_transform_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_TRANSFORM);
        size_t pos = sizeof(ComponentType);

        Matrix4f localMatrix;
        Matrix4f globalMatrix;

        memcpy(
            &localMatrix,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            &globalMatrix,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(Matrix4f)
        );

        *ppTransform = create_transform(
            entityID,
            Matrix4f(1.0f),
            pScene,
            true
        );

        (*ppTransform)->localMatrix = localMatrix;
        (*ppTransform)->globalMatrix = globalMatrix;
    }


    void deserialize(
        Scene* pScene,
        GUITransform** ppTransform,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_gui_transform_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_GUI_TRANSFORM);
        size_t pos = sizeof(ComponentType);

        Vector2f position;
        Vector2f scale;

        memcpy(
            &position,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(Vector2f)
        );
        pos += sizeof(Vector2f);

        memcpy(
            &scale,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(Vector2f)
        );

        *ppTransform = create_gui_transform(
            entityID,
            position,
            scale,
            pScene,
            true
        );
    }


    void deserialize(
        Scene* pScene,
        Parent** ppParent,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_parent_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_PARENT);
        size_t pos = sizeof(ComponentType);

        // TODO:
        //  Some system to find the existing parent Entity's UUID here OR
        //  wait until the parent entity gets constructed

        // Attempt to find the actual entity ID of the parent
        //  -> if not found?
        //      -> add to "requested list" which resolves this later in deserialization process
        UUID_t parentUUID = NULL_UUID;
        memcpy(
            &parentUUID,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(UUID_t)
        );
        entityID_t parentEntityID = pScene->getEntity(parentUUID).id;
        // TODO: Make below actually possible!
        if (parentEntityID == NULL_ENTITY_ID)
            pScene->addToDeserializationParentIDQuery(entityID, parentUUID);

        *ppParent = create_parent(entityID, parentEntityID, pScene, true);
    }


    void deserialize(
        Scene* pScene,
        Children** ppChildren,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_children_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_CHILDREN);
        size_t pos = sizeof(ComponentType);

        size_t childCount;
        memcpy(
            &childCount,
            reinterpret_cast<const char*>(pData) + pos,
            sizeof(size_t)
        );
        pos += sizeof(size_t);

        std::vector<UUID_t> requestedChildEntityUUIDs;
        for (size_t i = 0; i < childCount; ++i)
        {
            UUID_t childUUID = NULL_UUID;
            memcpy(
                &childUUID,
                reinterpret_cast<const char*>(pData) + pos,
                sizeof(UUID_t)
            );

            requestedChildEntityUUIDs.push_back(childUUID);
            pos += sizeof(UUID_t);
        }

        pScene->addToDeserializationChildrenIDQuery(entityID, requestedChildEntityUUIDs);

        std::vector<entityID_t> childEntityIDs(childCount);
        memset(childEntityIDs.data(), NULL_ENTITY_ID, sizeof(entityID_t) * childCount);
        Children* pChildren = create_children(entityID, childEntityIDs, pScene, true);
        ppChildren = &pChildren;
    }
}
