﻿#pragma once

#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/ComponentPool.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/systems/System.h"
#include "platypus/utils/Maths.h"

#include <unordered_map>
#include <vector>


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
        std::unordered_map<entityID_t, std::vector<entityID_t>> _entityChildMapping;
        std::vector<entityID_t> _freeEntityIDs;
        std::unordered_map<ComponentType, ComponentPool> _componentPools;

    public:
        EnvironmentProperties environmentProperties;

        Scene();
        virtual ~Scene();

        void* allocateComponent(entityID_t target, ComponentType componentType);

        entityID_t createEntity();
        Entity getEntity(entityID_t entity) const;
        void destroyEntity(entityID_t entityID);
        void destroyComponent(entityID_t entityID, ComponentType componentType);
        void addChild(entityID_t entityID, entityID_t childID);
        // NOTE: Could be optimized to return just ptr to first child and child count
        std::vector<entityID_t> getChildren(entityID_t entityID) const;

        // TODO: All getComponent things could be optimized?
        void* getComponent(ComponentType type, bool enableWarning=true);
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
        // Returns first component of "type" found in "entity"'s child entities
        void* getComponentInChildren(entityID_t entityID, ComponentType type);

        virtual void init() = 0;
        virtual void update() = 0;

        void addToComponentMask(entityID_t entityID, ComponentType componentType);
        // @param errLocation This can be used to tell what func caused this to error
        bool isValidEntity(entityID_t entityID, const std::string& errLocation) const;
        bool isValidComponent(ComponentType, const std::string& errLocation) const;
    };
}
