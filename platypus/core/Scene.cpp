#include "Scene.h"
#include "Application.h"
#include "Debug.h"
#include "platypus/ecs/components/Lights.h"


namespace platypus
{
    Scene::Scene()
    {
        size_t maxEntityCount = 1000000;

        _componentPools[ComponentType::COMPONENT_TYPE_TRANSFORM] = ComponentPool(
            sizeof(Transform), maxEntityCount, true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE] = ComponentPool(
            sizeof(Transform), maxEntityCount, true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_CAMERA] = ComponentPool(
            sizeof(Camera), 1, true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT] = ComponentPool(
            sizeof(DirectionalLight), 1, true
        );
    }

    Scene::~Scene()
    {
        std::unordered_map<ComponentType, ComponentPool>::iterator poolIterator;
        for (poolIterator = _componentPools.begin(); poolIterator != _componentPools.end(); ++poolIterator)
            poolIterator->second.freeStorage();

        _entities.clear();

        for (System* system : _systems)
            delete system;

        _systems.clear();

        _entityChildMapping.clear();
    }

    entityID_t Scene::createEntity()
    {
        Entity entity;
        if (!_freeEntityIDs.empty())
        {
            entityID_t freeID = _freeEntityIDs.back();
            _freeEntityIDs.pop_back();

            entity.id = freeID;
            _entities[freeID] = entity;
        }
        else
        {
            entity.id = _entities.size();
            _entities.push_back(entity);
        }
        return entity.id;
    }

    Entity Scene::getEntity(entityID_t entity) const
    {
        Entity outEntity;
        for (const Entity& e : _entities)
        {
            if (e.id == entity)
            {
                if (!isValidEntity(entity, "getEntity"))
                {
                    PLATYPUS_ASSERT(false);
                }
                else
                {
                    outEntity = e;
                }
                break;
            }
        }
        return outEntity;
    }

    // NOTE: Incomplete and not tested! Propably doesnt work!!
    // TODO: Finish and test this
    void Scene::destroyEntity(entityID_t entityID)
    {
        if (!isValidEntity(entityID, "destroyEntity"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        // Destroy/free all this entity's components
        uint64_t componentMask = _entities[entityID].componentMask;
        std::unordered_map<ComponentType, ComponentPool>::iterator poolsIt;
        for (poolsIt = _componentPools.begin(); poolsIt != _componentPools.end(); ++poolsIt)
        {
            if (componentMask & poolsIt->first)
                poolsIt->second.destroyComponent(entityID);
        }
        // Destroy/free entity itself
        _freeEntityIDs.push_back(entityID);
        _entities[entityID].clear();

        if (_entityChildMapping.find(entityID) != _entityChildMapping.end())
        {
            for (entityID_t childID : _entityChildMapping[entityID])
            {
                destroyEntity(childID);
            }
            _entityChildMapping.erase(entityID);
        }
    }

    void Scene::destroyComponent(entityID_t entityID, ComponentType componentType)
    {
        if (!isValidEntity(entityID, "destroyComponent"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        if (!isValidComponent(componentType, "destroyComponent"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }

        uint64_t componentMask = _entities[entityID].componentMask;
        if (componentMask & componentType)
            _componentPools[componentType].destroyComponent(entityID);
    }

    void Scene::addChild(entityID_t entityID, entityID_t childID)
    {
        if (!isValidEntity(entityID, "addChild"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        if (!isValidEntity(childID, "addChild Invalid child entity"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        _entities[childID].parentID = entityID;
        _entityChildMapping[entityID].push_back(childID);
    }

    // NOTE: Could be optimized to return just ptr to first child and child count
    std::vector<entityID_t> Scene::getChildren(entityID_t entityID) const
    {
        if (!isValidEntity(entityID, "getChildren"))
        {
            PLATYPUS_ASSERT(false);
            return {};
        }
        if (_entityChildMapping.find(entityID) == _entityChildMapping.end())
            return {};
        return _entityChildMapping.at(entityID);
    }

    void* Scene::getComponent(ComponentType type, bool enableWarning)
    {
        if (!isValidComponent(type, "getComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_componentPools[type].getComponentCount() == 0)
        {
            if (enableWarning)
            {
                Debug::log(
                    "Scene::getComponent "
                    "No components of type: " + component_type_to_string(type) + " found",
                    Debug::MessageType::PLATYPUS_WARNING
                );
            }
            return nullptr;
        }
        return _componentPools[type].first();
    }

    void* Scene::getComponent(
        entityID_t entityID,
        ComponentType type,
        bool nestedSearch,
        bool enableWarning
    )
    {
        if (!isValidEntity(entityID, "getComponent (1)"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_componentPools.find(type) == _componentPools.end())
        {
            Debug::log(
                "@Scene::getComponent (1)"
                "No component pool exists for component type: " + std::to_string(type),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if ((_entities[entityID].componentMask & (uint64_t)type) == (uint64_t)type)
        {
            // NOTE: Changed to using [] operator, below commented out previous way...
            return _componentPools[type][entityID];
            //return (Component*)componentPools[type].getComponent_DANGER(entityID);
        }
        if (!nestedSearch && enableWarning)
        {
            Debug::log(
                "@Scene::getComponent (1)"
                "Couldn't find component of type: " + std::to_string(type) + " "
                "from entity: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_WARNING
            );
        }
        return nullptr;
    }

    const void* Scene::getComponent(
        entityID_t entityID,
        ComponentType type,
        bool nestedSearch,
        bool enableWarning
    ) const
    {
        if (!isValidEntity(entityID, "getComponent (2)"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        std::unordered_map<ComponentType, ComponentPool>::const_iterator poolIt = _componentPools.find(type);
        if (poolIt == _componentPools.end())
        {
            Debug::log(
                "@Scene::getComponent (2)"
                "No component pool exists for component type: " + std::to_string(type),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if ((_entities[entityID].componentMask & (uint64_t)type) == (uint64_t)type)
        {
            // NOTE: Changed to using [] operator, below commented out previous way...
            return poolIt->second[entityID];
            //return (Component*)componentPools[type].getComponent_DANGER(entityID);
        }
        if (!nestedSearch && enableWarning)
        {
            Debug::log(
                "@Scene::getComponent (2)"
                "Couldn't find component of type: " + std::to_string(type) + " "
                "from entity: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_WARNING
            );
        }
        return nullptr;
    }

    // Returns first component of "type" found in "entity"'s child entities
    void* Scene::getComponentInChildren(entityID_t entityID, ComponentType type)
    {
        for (const entityID_t& child : _entityChildMapping[entityID])
        {
            void* pComponent = getComponent(child, type, true);
            if (pComponent)
                return pComponent;
        }
        Debug::log(
            "Scene::getComponentInChildren "
            "Couldn't find component of type: " + std::to_string(type) + " "
            "from child entities of entity: " + std::to_string(entityID),
            Debug::MessageType::PLATYPUS_MESSAGE
        );
        return nullptr;
    }

    Transform* Scene::createTransform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale
    )
    {
        if (!isValidEntity(target, "createTransform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_TRANSFORM;
        if (!isValidComponent(componentType, "createTransform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        Transform* pComponent = (Transform*)_componentPools[componentType].allocComponent(target);
        addToComponentMask(target, componentType);
        pComponent->globalMatrix = create_transformation_matrix(
            position,
            rotation,
            scale
        );

        return pComponent;
    }

    StaticMeshRenderable* Scene::createStaticMeshRenderable(
        entityID_t target,
        ID_t meshAssetID,
        ID_t textureAssetID
    )
    {
        if (!isValidEntity(target, "createStaticMeshRenderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE;
        if (!isValidComponent(componentType, "createStaticMeshRenderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        StaticMeshRenderable* pComponent = (StaticMeshRenderable*)_componentPools[componentType].allocComponent(target);
        addToComponentMask(target, componentType);
        pComponent->meshID = meshAssetID;
        pComponent->textureID = textureAssetID;

        return pComponent;
    }

    GUIRenderable* Scene::createGUIRenderable(
        entityID_t target,
        const Vector4f color
    )
    {
        if (!isValidEntity(target, "createGUIRenderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        if (!isValidComponent(componentType, "createGUIRenderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        GUIRenderable* pComponent = (GUIRenderable*)_componentPools[componentType].allocComponent(target);
        addToComponentMask(target, componentType);
        pComponent->color = color;
        return pComponent;
    }

    Camera* Scene::createCamera(
        entityID_t target,
        const Matrix4f& perspectiveProjectionMatrix
    )
    {
        if (!isValidEntity(target, "createCamera"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CAMERA;
        if (!isValidComponent(componentType, "createCamera"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        Camera* pComponent = (Camera*)_componentPools[componentType].allocComponent(target);
        addToComponentMask(target, componentType);
        pComponent->perspectiveProjectionMatrix = perspectiveProjectionMatrix;
        return pComponent;
    }

    DirectionalLight* Scene::createDirectionalLight(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color
    )
    {
        if (!isValidEntity(target, "createDirectionalLight"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT;
        if (!isValidComponent(componentType, "createDirectionalLight"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        DirectionalLight* pComponent = (DirectionalLight*)_componentPools[componentType].allocComponent(target);
        addToComponentMask(target, componentType);
        pComponent->direction = direction;
        pComponent->color = color;
        return pComponent;
    }

    void Scene::addToComponentMask(entityID_t entityID, ComponentType componentType)
    {
        if (!isValidEntity(entityID, "addToComponentMask"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        uint64_t componentTypeID = componentType;
        Entity& entity = _entities[entityID];
        // For now dont allow same component type multiple times in entity
        if (entity.componentMask & componentTypeID)
        {
            Debug::log(
                "@Scene::addToComponentMask "
                "Attempted to add component to entity: " + std::to_string(entityID) + " but "
                "it already has a component of this type: " + std::to_string(componentTypeID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        entity.componentMask |= componentTypeID;
    }

    bool Scene::isValidEntity(entityID_t entityID, const std::string& errLocation) const
    {
        bool success = true;
        if (entityID < 0 || entityID >= _entities.size())
            success = false;
        success = _entities[entityID].id != NULL_ENTITY_ID;
        if (!success)
        {
            Debug::log(
                "@Scene::" + errLocation + " "
                "Invalid entityID: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
        return success;
    }

    bool Scene::isValidComponent(ComponentType type, const std::string& errLocation) const
    {
        bool success = _componentPools.find(type) != _componentPools.end();
        if (!success)
        {
            Debug::log(
                "@Scene::" + errLocation + " "
                "Invalid component type: " + component_type_to_string(type),
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
        return success;
    }
}
