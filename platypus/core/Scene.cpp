#include "Scene.h"
#include "Application.h"
#include "Debug.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Lights.h"
#include "platypus/ecs/systems/SkeletalAnimationSystem.h"
#include "platypus/ecs/systems/TransformSystem.h"


namespace platypus
{
    Scene::Scene()
    {
        size_t maxEntityCount = 10000;

        _componentPools[ComponentType::COMPONENT_TYPE_TRANSFORM] = ComponentPool(
            ComponentType::COMPONENT_TYPE_TRANSFORM,
            sizeof(Transform),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_GUI_TRANSFORM] = ComponentPool(
            ComponentType::COMPONENT_TYPE_GUI_TRANSFORM,
            sizeof(GUITransform),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE] = ComponentPool(
            ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE,
            sizeof(StaticMeshRenderable),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE] = ComponentPool(
            ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE,
            sizeof(SkinnedMeshRenderable),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE] = ComponentPool(
            ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE,
            sizeof(TerrainMeshRenderable),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION] = ComponentPool(
            ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION,
            sizeof(SkeletalAnimation),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_GUI_RENDERABLE] = ComponentPool(
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE,
            sizeof(GUIRenderable),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_CAMERA] = ComponentPool(
            ComponentType::COMPONENT_TYPE_CAMERA,
            sizeof(Camera),
            1,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_LIGHT] = ComponentPool(
            ComponentType::COMPONENT_TYPE_LIGHT,
            sizeof(Light),
            1,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_PARENT] = ComponentPool(
            ComponentType::COMPONENT_TYPE_PARENT,
            sizeof(Parent),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_CHILDREN] = ComponentPool(
            ComponentType::COMPONENT_TYPE_CHILDREN,
            sizeof(Children),
            maxEntityCount,
            true
        );
        _componentPools[ComponentType::COMPONENT_TYPE_JOINT] = ComponentPool(
            ComponentType::COMPONENT_TYPE_JOINT,
            sizeof(SkeletonJoint),
            maxEntityCount,
            true
        );

        _systems.push_back(new SkeletalAnimationSystem);
        _systems.push_back(new TransformSystem);
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
    }

    void* Scene::allocateComponent(entityID_t target, ComponentType componentType)
    {
        if (!isValidComponent(componentType, "allocateComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _componentPools[componentType].allocComponent(target);
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
        std::unordered_map<ComponentType, ComponentPool>::iterator poolsIt;
        for (poolsIt = _componentPools.begin(); poolsIt != _componentPools.end(); ++poolsIt)
        {
            if (_entities[entityID].componentMask & poolsIt->first)
                poolsIt->second.destroyComponent(entityID);
        }
        // Destroy/free entity itself
        _freeEntityIDs.push_back(entityID);
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
        _componentPools[componentType].destroyComponent(entityID);
        _entities[entityID].componentMask &= ~componentType;
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

    const void* Scene::getComponent(ComponentType type, bool enableWarning) const
    {
        if (!isValidComponent(type, "getComponent"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        std::unordered_map<ComponentType, ComponentPool>::const_iterator it = _componentPools.find(type);
        if (it->second.getComponentCount() == 0)
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
        return it->second.first();
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
            return _componentPools[type][entityID];
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
        std::unordered_map<ComponentType, ComponentPool>::const_iterator poolIt = _componentPools.find(type);
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
            return poolIt->second[entityID];
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
