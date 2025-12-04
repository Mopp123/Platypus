#include "Renderable.h"
#include "platypus/core/Application.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    Renderable3D* create_renderable3D(
        entityID_t target,
        ID_t meshAssetID,
        ID_t materialAssetID
    )
    {
        Application* pApp = Application::get_instance();
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
        Vector2f textureOffset,
        uint32_t layer,
        std::wstring text
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
        pRenderable->textureOffset = textureOffset;
        pRenderable->layer = layer;
        // NOTE: Not sure if this is an issue
        if (!text.empty())
        {
            if (text.size() <= 32)
                pRenderable->text.resize(32);
            else
                pRenderable->text.resize(text.size());

            pRenderable->text = text;
        }

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
            { 0, 0 },
            0,
            L""
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
            { 1, 1, 1, 1 },
            { 0, 0 },
            0,
            L""
        );
    }
}
