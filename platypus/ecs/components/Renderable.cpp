#include "Renderable.h"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/assets/AssetManager.hpp"


namespace platypus
{
    Renderable3D* create_renderable3D(
        entityID_t target,
        ID_t meshAssetID,
        ID_t materialAssetID
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        const Mesh* pMesh = (const Mesh*)pAssetManager->getAsset(
            meshAssetID,
            AssetType::ASSET_TYPE_MESH
        );
        const Material* pMaterial = (const Material*)pAssetManager->getAsset(
            materialAssetID,
            AssetType::ASSET_TYPE_MATERIAL
        );
        if (pMesh->getType() == MeshType::MESH_TYPE_STATIC_INSTANCED && pMaterial->isTransparent())
        {
            Debug::log(
                "@create_renderable3D "
                "Mesh type was MESH_TYPE_STATIC_INSTANCED and Material was transparent. "
                "Instanced transparent renderables aren't currently supported!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        Scene* pScene = pApp->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_renderable3D"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_RENDERABLE3D;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_renderable3D "
                "Failed to allocate Renderable3D component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        Renderable3D* pRenderable = (Renderable3D*)pComponent;
        pRenderable->meshID = meshAssetID;
        pRenderable->materialID = materialAssetID;

        return pRenderable;
    }


    GUIRenderable* create_gui_renderable(
        entityID_t target,
        ID_t textureID,
        ID_t fontID,
        Vector4f color,
        Vector4f borderColor,
        float borderThickness,
        Vector2f textureOffset,
        uint32_t layer,
        bool isText,
        std::string text
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_gui_renderable(1)"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_gui_renderable(1) "
                "Failed to allocate GUIRenderable component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        GUIRenderable* pRenderable = (GUIRenderable*)pComponent;
        pRenderable->textureID = textureID;
        pRenderable->fontID = fontID;
        pRenderable->color = color;
        pRenderable->borderColor = borderColor;
        pRenderable->borderThickness = borderThickness;
        pRenderable->textureOffset = textureOffset;
        pRenderable->layer = layer;

        // UPDATE to below: Started calling Component constructors and destructors with
        // the new MemoryPool system, which should have fixed below issue!
        //  -> Still some kind of StringPool should probably be concidered for components that needs
        //  string members!
        //
        // NOTE: pRenderable was zero initialized
        // -> need some way to "allocate" the text
        // THIS IS CURRENTLY UNDEFINED BEHAVIOUR even it seems to work.
        // TODO: Some StringPool system for components to use that needs strings!
        //if (text.size() <= 32)
        //    pRenderable->text.resize(32);
        //else
        //    pRenderable->text.resize(text.size());

        pRenderable->isText = isText;
        pRenderable->text = text;

        return pRenderable;
    }

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        const Vector4f color
    )
    {
        return create_gui_renderable(
            target,
            NULL_ID,
            NULL_ID,
            color,
            { 0, 0, 0, 0 }, // border color
            0.0f, // border thickness
            { 0, 0 },
            0,
            false,
            ""
        );
    }

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        ID_t textureID
    )
    {
        return create_gui_renderable(
            target,
            textureID,
            NULL_ID,
            { 1, 1, 1, 1 }, // color
            { 0, 0, 0, 0 }, // border color
            0.0f, // border thickness
            { 0, 0 },
            0,
            false,
            ""
        );
    }
}
