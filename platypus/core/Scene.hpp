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

        uint32_t _entityUUIDPool = 0;

        std::vector<System*> _systems;
        std::vector<Entity> _entities;
        std::unordered_map<std::string, size_t> _nameEntityMapping;

        std::queue<entityID_t> _freeEntityIDs;
        // NOTE: I don't like these being heap allocated, but want to get this just working for now...
        std::unordered_map<ComponentType, MemoryPool*> _componentPools;

        std::unordered_map<UUID_t, std::vector<EntityError>> _entityErrors;
        const std::unordered_map<EntityErrorType, void(*)(Scene*, EntityError)> _entityErrorFixMapping = {
            { EntityErrorType::COMPONENT_RENDERABLE3D_MESH_UNAVAILABLE, &handle_mesh_unavailable_error },
            { EntityErrorType::COMPONENT_RENDERABLE3D_MATERIAL_UNAVAILABLE, &handle_material_unavailable_error },
            { EntityErrorType::COMPONENT_RENDERABLE3D_INCOMPATIBLE_MESH_MATERIAL, &handle_incompatible_mesh_material_error }
        };

        entityID_t _activeCameraEntity = NULL_ENTITY_ID;

        // Needed for Parent and Children components' deserialization
        std::unordered_map<entityID_t, UUID_t> _parentComponentsToFinalize;
        std::unordered_map<entityID_t, std::vector<UUID_t>> _childrenComponentsToFinalize;

    public:
        EnvironmentProperties environmentProperties;

        Scene();
        virtual ~Scene();

        void* allocateComponent(entityID_t target, ComponentType componentType);

        entityID_t createEntity(const std::string& name = "", UUID_t explicitUUID = NULL_UUID);
        Entity getEntity(entityID_t entity) const;
        Entity getEntity(UUID_t entityUUID) const;
        Entity getEntity(const std::string& name) const;
        std::string getEntityName(entityID_t entity) const;
        void setEntityName(const std::string& currentName, const std::string& newName);
        void setEntityActive(entityID_t entity, bool arg);
        bool isEntityActive(entityID_t entity) const;
        bool entityExists(entityID_t entity) const;
        bool entityExists(const std::string& name) const;
        inline const std::vector<Entity>& getEntities() const { return _entities; }
        // TODO:
        // *Make getEntityNames() safer!
        // *Get rid of getEntityNames() when figuring out some more coherent system with
        // entity names!
        // NOTE: getEntityNames() is slow as fuck! DO NOT RELY ON THIS AT "SCENE RUNTIME"
        std::vector<std::string> getEntityNames() const;
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

        // NOTE: WARNING! NOT THREAD SAFE!
        void insertError(UUID_t entityUUID, EntityError error);
        const std::unordered_map<UUID_t, std::vector<EntityError>>& getErrors();
        void clearErrors();

        // These both are supposed to resolve all Parent and Children components' actual entityID_ts
        // after all entities have been deserialized!
        void addToDeserializationParentIDQuery(
            entityID_t target,
            UUID_t parentEntityUUID
        );
        void addToDeserializationChildrenIDQuery(
            entityID_t target,
            const std::vector<UUID_t>& childUUIDs
        );

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
        entityID_t deserialize(
            const std::vector<char>& serializedData,
            size_t bufferReadPos,
            size_t& bufferReadEndPos
        );

        std::vector<entityID_t> deserialize(
            const std::vector<char>& serializedData,
            size_t serializedDataPos
        );

        void finalizeDeserialization();

        virtual void init() = 0;
        virtual void update() = 0;

        inline entityID_t getActiveCameraEntity() const { return _activeCameraEntity; }
    };
}
