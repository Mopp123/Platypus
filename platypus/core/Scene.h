#pragma once

#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/ComponentPool.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Lights.h"

#include "platypus/ecs/systems/System.h"

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

        Transform* createTransform(
            entityID_t target,
            const Vector3f& position,
            const Quaternion& rotation,
            const Vector3f& scale
        );

        StaticMeshRenderable* createStaticMeshRenderable(
            entityID_t target,
            ID_t meshAssetID,
            ID_t textureAssetID
        );

        Camera* createCamera(
            entityID_t target,
            const Matrix4f& perspectiveProjectionMatrix
        );

        DirectionalLight* createDirectionalLight(
            entityID_t target,
            const Vector3f& direction,
            const Vector3f& color
        );

        virtual void init() = 0;
        virtual void update() = 0;

    private:
        void addToComponentMask(entityID_t entityID, ComponentType componentType);
        // @param errLocation This can be used to tell what func caused this to error
        bool isValidEntity(entityID_t entityID, const std::string& errLocation) const;
        bool isValidComponent(ComponentType, const std::string& errLocation) const;
    };
}
