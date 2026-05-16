#pragma once

#include "platypus/ecs/Entity.hpp"
#include "Memory.hpp"
#include "platypus/ecs/components/Component.hpp"
#include "platypus/ecs/systems/System.hpp"
#include "platypus/utils/Maths.hpp"

#include <unordered_map>
#include <vector>
#include <queue>


namespace platypus
{
    class SceneManager;

    struct EnvironmentProperties
    {
        Vector3f ambientColor = Vector3f(0, 0, 0);
        Vector4f clearColor = Vector4f(0, 0, 1, 1);
    };

    class Scene
    {
    private:
        friend class SceneManager;

        std::vector<System*> _systems;
        std::vector<Entity> _entities;

        std::queue<entityID_t> _freeEntityIDs;
        // NOTE: I don't like these being heap allocated, but want to get this just working for now...
        std::unordered_map<ComponentType, MemoryPool*> _componentPools;

        entityID_t _activeCameraEntity = NULL_ENTITY_ID;

    public:
        EnvironmentProperties environmentProperties;

        Scene();
        virtual ~Scene();

        void* allocateComponent(entityID_t target, ComponentType componentType);

        entityID_t createEntity(UUID_t explicitUUID = NULL_UUID);
        Entity getEntity(entityID_t entity) const;
        Entity getEntity(UUID_t entityUUID) const;
        void setEntityActive(entityID_t entity, bool arg);
        bool isEntityActive(entityID_t entity) const;
        bool entityExists(entityID_t entity) const;
        inline const std::vector<Entity>& getEntities() const { return _entities; }
        void destroyEntity(entityID_t entityID);
        void destroyComponent(entityID_t entityID, ComponentType componentType);

        // TODO: All getComponent things could be optimized?
        void* getComponent(ComponentType type, bool enableWarning=true);
        const void* getComponent(ComponentType type, bool enableWarning=true) const;
        void* getComponent(
            entityID_t entityID,
            ComponentType type,
            bool nestedSearch = false,
            bool enableWarning = true
        );
        const void* getComponent(
            entityID_t entityID,
            ComponentType type,
            bool nestedSearch = false,
            bool enableWarning = true
        ) const;

        std::unordered_map<ComponentType, const void*> getComponents(entityID_t entityID) const;

        void addToComponentMask(entityID_t entityID, ComponentType componentType);
        void setComponentMask(entityID_t entityID, uint64_t mask);
        // @param errLocation This can be used to tell what func caused this to error
        bool isValidEntity(entityID_t entityID, const std::string& errLocation) const;
        bool isValidComponent(ComponentType, const std::string& errLocation) const;

        void setActiveCameraEntity(entityID_t entityID);

        // Puts all entities and their components into buffer that can be saved on disk
        std::vector<char> serialize(
            const std::vector<entityID_t>& toSerialize
        );

        // Returns the position after the header in serializedData
        // (the "actual position", including the entered serializedDataPos)
        size_t deserializeHeader(
            const std::vector<char>& serializedData,
            size_t serializedDataPos,
            size_t* pEntityCount
        ) const;


        // Creates scene according to inputted serialized buffer.
        // NOTE: The input buffer here is currently intended to contain all assets as well
        // TODO: Why the fuck does the above need to be so? -> rather take just the portion containing
        // the entity and component stuff?

        entityID_t deserialize(
            const std::vector<char>& serializedData,
            size_t bufferReadPos,
            size_t& bufferReadEndPos
        );

        std::vector<entityID_t> deserialize(
            const std::vector<char>& serializedData,
            size_t serializedDataPos
        );

        virtual void init() = 0;
        virtual void update() = 0;

        inline entityID_t getActiveCameraEntity() const { return _activeCameraEntity; }
    };
}
