#include "Renderable.h"
#include "platypus/core/Application.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    StaticMeshRenderable* create_static_mesh_renderable(
        entityID_t target,
        ID_t meshAssetID,
        ID_t materialAssetID
    )
    {
        Application* pApp = Application::get_instance();
        Scene* pScene = pApp->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_static_mesh_renderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_static_mesh_renderable "
                "Failed to allocate StaticMeshRenderable component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        StaticMeshRenderable* pRenderable = (StaticMeshRenderable*)pComponent;
        pRenderable->meshID = meshAssetID;
        pRenderable->materialID = materialAssetID;

        // Create material pipeline and initial descriptor sets
        AssetManager& assetManager = Application::get_instance()->getAssetManager();
        Material* pMaterial = (Material*)assetManager.getAsset(materialAssetID, AssetType::ASSET_TYPE_MATERIAL);
        if (!pMaterial->getPipelineData())
        {
            Mesh* pMesh = (Mesh*)assetManager.getAsset(meshAssetID, AssetType::ASSET_TYPE_MESH);
            Debug::log(
                "@create_static_mesh_renderable "
                "Material didn't have pipeline data yet -> creating..."
            );
            pMaterial->createPipeline(pMesh->getVertexBufferLayout(), false);
            pMaterial->createDescriptorSets();
        }

        return pRenderable;
    }

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        const Vector4f color
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_gui_renderable"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_gui_renderable "
                "Failed to allocate GUIRenderable component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        GUIRenderable* pRenderable = (GUIRenderable*)pComponent;
        pRenderable->color = color;
        return pRenderable;
    }
}
