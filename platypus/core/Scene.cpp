#include "Scene.hpp"
#include "Application.hpp"
#include "Debug.hpp"

#include "platypus/ecs/components/ComponentPool.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/SkeletalAnimation.hpp"
#include "platypus/ecs/components/Camera.hpp"
#include "platypus/ecs/components/Lights.hpp"
#include "platypus/ecs/systems/SkeletalAnimationSystem.hpp"
#include "platypus/ecs/systems/TransformSystem.hpp"
#include "platypus/ecs/systems/LightSystem.hpp"


namespace platypus
{
    Scene::Scene()
    {
        size_t maxPoolLength = 10000;

        _componentPools[ComponentType::COMPONENT_TYPE_TRANSFORM] = new ComponentPool<Transform>(
            sizeof(Transform),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_GUI_TRANSFORM] = new ComponentPool<GUITransform>(
            sizeof(GUITransform),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_RENDERABLE3D] = new ComponentPool<Renderable3D>(
            sizeof(Renderable3D),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION] = new ComponentPool<SkeletalAnimation>(
            sizeof(SkeletalAnimation),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_GUI_RENDERABLE] = new ComponentPool<GUIRenderable>(
            sizeof(GUIRenderable),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_CAMERA] = new ComponentPool<Camera>(
            sizeof(Camera),
            1,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_LIGHT] = new ComponentPool<Light>(
            sizeof(Light),
            1,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_PARENT] = new ComponentPool<Parent>(
            sizeof(Parent),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_CHILDREN] = new ComponentPool<Children>(
            sizeof(Children),
            maxPoolLength,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_JOINT] = new ComponentPool<SkeletonJoint>(
            sizeof(SkeletonJoint),
            maxPoolLength,
            true
        );

        _systems.push_back(new SkeletalAnimationSystem);
        _systems.push_back(new TransformSystem);
        _systems.push_back(new LightSystem);
    }

    Scene::~Scene()
    {
        std::unordered_map<ComponentType, MemoryPool*>::iterator poolIterator;
        for (poolIterator = _componentPools.begin(); poolIterator != _componentPools.end(); ++poolIterator)
            poolIterator->second->freeStorage();

        _entities.clear();

        for (System* system : _systems)
            delete system;

        _systems.clear();
    }

    void* Scene::allocateComponent(entityID_t target, ComponentType componentType)
    {
        if (!isValidComponent(componentType, "allocateComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _componentPools[componentType]->occupy(&target);
    }

    entityID_t Scene::createEntity()
    {
        Entity entity;
        if (!_freeEntityIDs.empty())
        {
            entityID_t freeID = _freeEntityIDs.front();
            _freeEntityIDs.pop();

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

    // NOTE: Previously looped all entities, this should be faster
    // BUT not sure if this is safe enough, especially when deleting
    // entities from "between" entities.
    Entity Scene::getEntity(entityID_t entity) const
    {
        if (entity < _entities.size())
        {
            if (_entities[entity].id != NULL_ENTITY_ID)
                return _entities[entity];
        }
        return { };
    }

    void Scene::setEntityActive(entityID_t entity, bool arg)
    {
        if (entity < _entities.size())
        {
            if (_entities[entity].id != NULL_ENTITY_ID)
            {
               _entities[entity].active = arg;
            }
            #ifdef PLATYPUS_DEBUG
            else
            {
                Debug::log(
                    "Entity with ID: " + std::to_string(entity) + " was marked as NULL_ENTITY_ID! "
                    "The entity may have already been destroyed.",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
        }
        #ifdef PLATYPUS_DEBUG
        else
        {
            Debug::log(
                "Entity ID: " + std::to_string(entity) + " out of bounds!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        #endif
    }

    bool Scene::isEntityActive(entityID_t entity) const
    {
        if (entity < _entities.size())
        {
            if (_entities[entity].id != NULL_ENTITY_ID)
            {
               return _entities[entity].active;
            }
            #ifdef PLATYPUS_DEBUG
            else
            {
                Debug::log(
                    "Entity with ID: " + std::to_string(entity) + " was marked as NULL_ENTITY_ID! "
                    "The entity may have already been destroyed.",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
        }
        #ifdef PLATYPUS_DEBUG
        else
        {
            Debug::log(
                "Entity ID: " + std::to_string(entity) + " out of bounds!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        #endif
        return false;
    }

    bool Scene::entityExists(entityID_t entity) const
    {
        if (entity < 0 || entity >= _entities.size())
            return false;
        return _entities[entity].id != NULL_ENTITY_ID;
    }

    void Scene::destroyEntity(entityID_t entityID)
    {
        if (!isValidEntity(entityID, "destroyEntity"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }
        // Check if entity has parent -> remove it from parent's child list
        Parent* pParent = (Parent*)getComponent(
            entityID,
            ComponentType::COMPONENT_TYPE_PARENT,
            false,
            false
        );
        if (pParent)
        {
            remove_child(pParent->entityID, entityID);
        }
        // Check if entity has children -> destroy those first
        Children* pChildren = (Children*)getComponent(
            entityID,
            ComponentType::COMPONENT_TYPE_CHILDREN,
            false,
            false
        );
        if (pChildren)
        {
            for (size_t i = 0; i < pChildren->count; ++i)
            {
                destroyEntity(pChildren->entityIDs[i]);
            }
        }

        // Destroy/free all this entity's components
        std::unordered_map<ComponentType, MemoryPool*>::iterator poolsIt;
        for (poolsIt = _componentPools.begin(); poolsIt != _componentPools.end(); ++poolsIt)
        {
            if (_entities[entityID].componentMask & poolsIt->first)
                poolsIt->second->clearStorage(&entityID);
        }
        // Destroy/free entity itself
        _freeEntityIDs.push(entityID);
        _entities[entityID].clear();
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
        if ((componentMask & componentType) != componentType)
        {
            Debug::log(
                "Scene::destroyComponent "
                "No component of type: " + component_type_to_string(componentType) + " found "
                "from entity: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _componentPools[componentType]->clearStorage(&entityID);
        _entities[entityID].componentMask &= ~componentType;
    }

    void* Scene::getComponent(ComponentType type, bool enableWarning)
    {
        if (!isValidComponent(type, "getComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (_componentPools[type]->getOccupiedCount() == 0)
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
        return _componentPools[type]->any();
    }

    const void* Scene::getComponent(ComponentType type, bool enableWarning) const
    {
        if (!isValidComponent(type, "getComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        std::unordered_map<ComponentType, MemoryPool*>::const_iterator it = _componentPools.find(type);
        if (it->second->getOccupiedCount() == 0)
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
        return it->second->any();
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
                "@Scene::getComponent (1) "
                "No component pool exists for component type: " + component_type_to_string(type),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if ((_entities[entityID].componentMask & (uint64_t)type) == (uint64_t)type)
        {
            // NOTE: Changed to using [] operator, below commented out previous way...
            return _componentPools[type]->getElement(&entityID);
            //return (Component*)componentPools[type].getComponent_DANGER(entityID);
        }
        if (!nestedSearch && enableWarning)
        {
            Debug::log(
                "@Scene::getComponent (1) "
                "Couldn't find component of type: " + component_type_to_string(type) + " "
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
        std::unordered_map<ComponentType, MemoryPool*>::const_iterator poolIt = _componentPools.find(type);
        if (poolIt == _componentPools.end())
        {
            Debug::log(
                "@Scene::getComponent (2) "
                "No component pool exists for component type: " + std::to_string(type),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if ((_entities[entityID].componentMask & (uint64_t)type) == (uint64_t)type)
        {
            // NOTE: Changed to using [] operator, below commented out previous way...
            return poolIt->second->getElement(&entityID);
            //return (Component*)componentPools[type].getComponent_DANGER(entityID);
        }
        if (!nestedSearch && enableWarning)
        {
            Debug::log(
                "@Scene::getComponent (2) "
                "Couldn't find component of type: " + component_type_to_string(type) + " "
                "from entity: " + std::to_string(entityID),
                Debug::MessageType::PLATYPUS_WARNING
            );
        }
        return nullptr;
    }

    std::unordered_map<ComponentType, const void*> Scene::getComponents(entityID_t entityID) const
    {
        const Entity entity = getEntity(entityID);
        const uint64_t componentMask = entity.componentMask;
        std::unordered_map<ComponentType, const void*> components;
        if (componentMask & ComponentType::COMPONENT_TYPE_CAMERA)
            components[ComponentType::COMPONENT_TYPE_CAMERA] = getComponent(entityID, ComponentType::COMPONENT_TYPE_CAMERA);

        if (componentMask & ComponentType::COMPONENT_TYPE_RENDERABLE3D)
            components[ComponentType::COMPONENT_TYPE_RENDERABLE3D] = getComponent(entityID, ComponentType::COMPONENT_TYPE_RENDERABLE3D);

        if (componentMask & ComponentType::COMPONENT_TYPE_GUI_RENDERABLE)
            components[ComponentType::COMPONENT_TYPE_GUI_RENDERABLE] = getComponent(entityID, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE);

        if (componentMask & ComponentType::COMPONENT_TYPE_TRANSFORM)
            components[ComponentType::COMPONENT_TYPE_TRANSFORM] = getComponent(entityID, ComponentType::COMPONENT_TYPE_TRANSFORM);

        if (componentMask & ComponentType::COMPONENT_TYPE_GUI_TRANSFORM)
            components[ComponentType::COMPONENT_TYPE_GUI_TRANSFORM] = getComponent(entityID, ComponentType::COMPONENT_TYPE_GUI_TRANSFORM);

        if (componentMask & ComponentType::COMPONENT_TYPE_PARENT)
            components[ComponentType::COMPONENT_TYPE_PARENT] = getComponent(entityID, ComponentType::COMPONENT_TYPE_PARENT);

        if (componentMask & ComponentType::COMPONENT_TYPE_CHILDREN)
            components[ComponentType::COMPONENT_TYPE_CHILDREN] = getComponent(entityID, ComponentType::COMPONENT_TYPE_CHILDREN);

        if (componentMask & ComponentType::COMPONENT_TYPE_LIGHT)
            components[ComponentType::COMPONENT_TYPE_LIGHT] = getComponent(entityID, ComponentType::COMPONENT_TYPE_LIGHT);

        if (componentMask & ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION)
            components[ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION] = getComponent(entityID, ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION);

        if (componentMask & ComponentType::COMPONENT_TYPE_JOINT)
            components[ComponentType::COMPONENT_TYPE_JOINT] = getComponent(entityID, ComponentType::COMPONENT_TYPE_JOINT);

        return components;
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

    void Scene::setActiveCameraEntity(entityID_t entityID)
    {
        PLATYPUS_ASSERT(isValidEntity(entityID, "Scene::setActiveCameraEntity"));
        const uint64_t requiredComponentMask = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_CAMERA;
        Entity entity = getEntity(entityID);
        if ((entity.componentMask & requiredComponentMask) == 0)
        {
            Debug::log(
                "@Scene::setActiveCameraEntity "
                "Entity: " + std::to_string(entityID) + " had inefficient component mask "
                "for being active camera!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        _activeCameraEntity = entityID;
    }
}
