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
        _entityUUIDPool = UUID::occupy_pool();
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

        UUID::erase_pool(_entityUUIDPool);
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

    entityID_t Scene::createEntity(const std::string& name, UUID_t explicitUUID)
    {
        Entity entity;
        UUID_t uuid = NULL_UUID;
        if (explicitUUID != NULL_UUID)
        {
            if (UUID::exists(explicitUUID, _entityUUIDPool))
            {
                Debug::log(
                    "Entity UUID " + std::to_string(explicitUUID) + " already exists "
                    "in UUID pool " + std::to_string(_entityUUIDPool),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            uuid = explicitUUID;
            UUID::occupy(uuid, _entityUUIDPool);
        }
        else
        {
            uuid = UUID::generate(_entityUUIDPool);
        }

        PLATYPUS_ASSERT(uuid != NULL_UUID);
        entity.uuid = uuid;

        if (!name.empty())
        {
            if (_nameEntityMapping.find(name) != _nameEntityMapping.end())
            {
                Debug::log(
                    "Entity name: " + name + " already exists!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            // Not allowing spaces in name atm!
            if (name.find(" ") != std::string::npos)
            {
                Debug::log(
                    "Invalid entity name: " + name + " "
                    "name can't currently contain any spaces!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

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

        if (!name.empty())
            _nameEntityMapping[name] = entity.id;

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

    Entity Scene::getEntity(UUID_t entityUUID) const
    {
        for (const Entity& entity : _entities)
        {
            if (entity.uuid == entityUUID)
                return entity;
        }
        return { };
    }

    Entity Scene::getEntity(const std::string& name) const
    {
        std::unordered_map<std::string, size_t>::const_iterator it = _nameEntityMapping.find(name);
        if (it != _nameEntityMapping.end())
            return getEntity(static_cast<entityID_t>(it->second));

        return { };
    }

    std::string Scene::getEntityName(entityID_t entity)
    {
        if (entity == NULL_ENTITY_ID)
            return "";

        std::unordered_map<std::string, size_t>::const_iterator it;
        for (it = _nameEntityMapping.begin(); it != _nameEntityMapping.end(); ++it)
        {
            if (it->second == static_cast<size_t>(entity))
                return it->first;
        }
        return "";
    }

    void Scene::setEntityName(const std::string& currentName, const std::string& newName)
    {
        // Not allowing spaces in name atm!
        if (newName.find(" ") != std::string::npos)
        {
            Debug::log(
                "Invalid entity name: " + newName + " "
                "name can't currently contain any spaces!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        #ifdef PLATYPUS_DEBUG
        if (_nameEntityMapping.find(newName) != _nameEntityMapping.end())
        {
            Debug::log(
                "Entity name " + newName + " already exists!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (_nameEntityMapping.find(currentName) == _nameEntityMapping.end())
        {
            Debug::log(
                "No entity name " + currentName + " found!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        #endif
        size_t entityIndex = _nameEntityMapping[currentName];
        _nameEntityMapping[newName] = entityIndex;
        _nameEntityMapping.erase(currentName);
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

    bool Scene::entityExists(const std::string& name) const
    {
        return _nameEntityMapping.find(name) != _nameEntityMapping.end();
    }

    std::vector<std::string> Scene::getEntityNames() const
    {
        std::vector<std::string> names;
        std::unordered_map<std::string, size_t>::const_iterator it;
        for (it = _nameEntityMapping.begin(); it != _nameEntityMapping.end(); ++it)
            names.push_back(it->first);

        return names;
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
        _entities[entityID].clear(_entityUUIDPool);

        // Free entity name
        // TODO: Optimize!
        std::unordered_map<std::string, size_t>::const_iterator nameIt;
        std::string foundName;
        for (nameIt = _nameEntityMapping.begin(); nameIt != _nameEntityMapping.end(); ++nameIt)
        {
            if (nameIt->second == entityID)
            {
                foundName = nameIt->first;
                break;
            }
        }
        if (!foundName.empty())
            _nameEntityMapping.erase(foundName);
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

    void Scene::setComponentMask(entityID_t entityID, uint64_t mask)
    {
        if (!isValidEntity(entityID, "setComponentMask"))
        {
            PLATYPUS_ASSERT(false);
            return;
        }

        // Not sure how this could happen, but just in case...
        if (_entities[entityID].id != entityID)
        {
            Debug::log(
                "Now u done fucked up xDdd",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _entities[entityID].componentMask = mask;
    }

    bool Scene::isValidEntity(entityID_t entityID, const std::string& errLocation) const
    {
        bool success = true;
        if (entityID < 0 || entityID >= _entities.size())
            return false;
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

    void Scene::insertError(UUID_t entityUUID, EntityError error)
    {
        Entity entity = getEntity(entityUUID);
        if (entity.uuid == NULL_UUID)
        {
            Debug::log(
                "Entity with UUID: " + std::to_string(entityUUID) + " not found!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            return;
        }
        _entityErrors[entityUUID].push_back(error);
    }

    const std::unordered_map<UUID_t, std::vector<EntityError>>& Scene::getErrors()
    {
        return _entityErrors;
    }

    void Scene::clearErrors()
    {
        _entityErrors.clear();
    }

    std::vector<char> Scene::serialize(
        const std::vector<entityID_t>& toSerialize
    )
    {
        std::vector<char> serializedData(sizeof(uint32_t));
        const uint32_t entityCount = static_cast<const uint32_t>(toSerialize.size());
        memcpy(serializedData.data(), &entityCount, sizeof(uint32_t));
        size_t pos = sizeof(uint32_t);

        for (entityID_t entityID : toSerialize)
        {
            const Entity& entity = getEntity(entityID);
            std::vector<char> entityData = serialize_entity(entity, getEntityName(entityID));
            serializedData.resize(serializedData.size() + entityData.size());
            memcpy(serializedData.data() + pos, entityData.data(), entityData.size());
            pos += entityData.size();

            std::unordered_map<ComponentType, const void*> components = getComponents(entityID);
            Debug::log(
                "___TEST___serializing " + std::to_string(components.size()) + " components "
                "for entity " + std::to_string(entityID)
            );
            std::unordered_map<ComponentType, const void*>::const_iterator it;
            for (it = components.begin(); it != components.end(); ++it)
            {
                Debug::log(
                    "___TEST___serializing component: " + component_type_to_string(it->first)
                );
                const size_t serializedComponentSize = get_component_serialized_size(it->first);
                serializedData.resize(serializedData.size() + serializedComponentSize);
                // TODO: UNFUCK THIS SHIT!
                std::vector<char> componentData = serialize_component(
                    it->first,
                    get_component_size(it->first),
                    it->second
                );
                PLATYPUS_ASSERT(componentData.size() == serializedComponentSize);
                memcpy(serializedData.data() + pos, componentData.data(), componentData.size());

                pos += componentData.size();
            }
        }
        return serializedData;
    }

    size_t Scene::deserializeHeader(
        const std::vector<char>& serializedData,
        size_t serializedDataPos,
        size_t* pEntityCount
    ) const
    {
        PLATYPUS_ASSERT((serializedDataPos + serialized_entities_header_size) <= serializedData.size());
        const char* pData = serializedData.data() + serializedDataPos;
        uint32_t entityCount = 0;
        memcpy(&entityCount, pData, serialized_entities_header_size);
        if (pEntityCount) *pEntityCount = static_cast<size_t>(entityCount);
        return serializedDataPos + serialized_entities_header_size;
    }

    entityID_t Scene::deserialize(
        const std::vector<char>& serializedData,
        size_t bufferReadPos,
        size_t& bufferReadEndPos
    )
    {
        PLATYPUS_ASSERT((bufferReadPos + serialized_entity_size) <= serializedData.size());
        const char* pData = serializedData.data();
        Entity entity;
        deserialize_entity(
            this,
            entity,
            serialized_entity_size,
            pData + bufferReadPos
        );
        bufferReadPos += serialized_entity_size;

        size_t componentCount = get_component_count(entity.componentMask);
        for (size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
        {
            PLATYPUS_ASSERT(bufferReadPos < serializedData.size());
            ComponentType componentType;
            memcpy(
                &componentType,
                pData + bufferReadPos,
                sizeof(ComponentType)
            );
            size_t serializedComponentSize = get_component_serialized_size(componentType);

            void* pComponent = nullptr;
            deserialize_component(
                this,
                componentType,
                &pComponent,
                entity.id,
                serializedComponentSize,
                pData + bufferReadPos
            );
            bufferReadPos += serializedComponentSize;
        }
        bufferReadEndPos = bufferReadPos;
        return entity.id;
    }

    std::vector<entityID_t> Scene::deserialize(
        const std::vector<char>& serializedData,
        size_t serializedDataPos
    )
    {
        PLATYPUS_ASSERT((serializedDataPos + sizeof(uint32_t)) <= serializedData.size());
        const char* pData = serializedData.data() + serializedDataPos;
        uint32_t entityCount = 0;
        memcpy(
            &entityCount,
            pData,
            sizeof(uint32_t)
        );

        size_t pos = sizeof(uint32_t);
        std::vector<entityID_t> entities(entityCount);
        for (size_t i = 0; i < entityCount; ++i)
        {
            Entity entity;
            deserialize_entity(
                this,
                entity,
                serialized_entity_size,
                reinterpret_cast<const char*>(pData) + pos
            );
            pos += serialized_entity_size;

            size_t componentCount = get_component_count(entity.componentMask);
            for (size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
            {
                ComponentType componentType;
                memcpy(
                    &componentType,
                    reinterpret_cast<const char*>(pData) + pos,
                    sizeof(ComponentType)
                );
                size_t serializedComponentSize = get_component_serialized_size(componentType);

                void* pComponent = nullptr;
                deserialize_component(
                    this,
                    componentType,
                    &pComponent,
                    entity.id,
                    serializedComponentSize,
                    reinterpret_cast<const char*>(pData) + pos
                );
                pos += serializedComponentSize;
            }
            entities[i] = entity.id;
        }
        return entities;
    }
}
