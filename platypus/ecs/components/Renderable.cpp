#include "Renderable.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/assets/AssetManager.hpp"


namespace platypus
{
    Renderable3D* create_renderable3D(
        entityID_t target,
        ID_t meshAssetID,
        ID_t materialAssetID,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        const Mesh* pMesh = (const Mesh*)pAssetManager->getAsset(
            meshAssetID,
            AssetType::ASSET_TYPE_MESH
        );
        PLATYPUS_ASSERT(pMesh);

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

        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = pApp->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_renderable3D"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_RENDERABLE3D;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

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
        std::string text,
        Scene* pScene
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_gui_renderable(1)"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        pUseScene->addToComponentMask(target, componentType);
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
        const Vector4f color,
        Scene* pScene
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
            "",
            pScene
        );
    }


    GUIRenderable* create_gui_renderable(
        entityID_t target,
        ID_t textureID,
        Scene* pScene
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
            "",
            pScene
        );
    }


    std::vector<char> serialize(const Renderable3D* pRenderable)
    {
        std::vector<char> serializedData(serialized_renderable3D_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_RENDERABLE3D;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pRenderable->meshID),
            sizeof(ID_t)
        );
        pos += sizeof(ID_t);

        memcpy(
            serializedData.data() + pos,
            &(pRenderable->materialID),
            sizeof(ID_t)
        );

        return serializedData;
    }


    void deserialize(
        Scene* pScene,
        Renderable3D** ppRenderable,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_renderable3D_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_RENDERABLE3D);
        size_t pos = sizeof(ComponentType);

        ID_t meshID ;
        ID_t materialID;

        memcpy(
            &meshID,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(ID_t)
        );
        pos += sizeof(ID_t);

        memcpy(
            &materialID,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(ID_t)
        );

        *ppRenderable = create_renderable3D(
            entityID,
            meshID,
            materialID,
            pScene,
            true
        );
    }
}
