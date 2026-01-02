#pragma once

#include "platypus/Platypus.h"
#include "platypus/utils/Maths.h"


class BaseScene : public platypus::Scene
{
private:
    class SceneWindowResizeEvent : public platypus::WindowResizeEvent
    {
    public:
        platypus::Scene* _pScene = nullptr;
        entityID_t _cameraEntity;
        SceneWindowResizeEvent(platypus::Scene* pScene, entityID_t cameraEntity) :
            _pScene(pScene),
            _cameraEntity(cameraEntity)
        {}
        virtual void func(int w, int h);
    };

protected:
    platypus::CameraController _cameraController;
    entityID_t _cameraEntity = NULL_ENTITY_ID;
    entityID_t _lightEntity = NULL_ENTITY_ID;

public:
    BaseScene();
    virtual ~BaseScene();
    void initBase();
    void updateBase();

    std::vector<ID_t> loadTextures(
        platypus::AssetManager* pAssetManager,
        platypus::ImageFormat imageFormat,
        platypus::TextureSampler sampler,
        std::vector<std::string> filepaths
    );

    platypus::Material* createMeshMaterial(
        platypus::AssetManager* pAssetManager,
        std::string textureFilepath,
        bool repeatTexture = false,
        bool castShadows = false,
        bool receiveShadows = false,
        bool transparent = false
    );

    entityID_t createStaticMeshEntity(
        const platypus::Vector3f& position,
        const platypus::Quaternion& rotation,
        const platypus::Vector3f& scale,
        ID_t meshAssetID,
        ID_t materialAssetID
    );

    entityID_t createSkinnedMeshEntity(
        const platypus::Vector3f& position,
        const platypus::Quaternion& rotation,
        const platypus::Vector3f& scale,
        platypus::Mesh* pMesh,
        platypus::SkeletalAnimationData* pAnimationAsset,
        ID_t materialAssetID,
        std::vector<entityID_t>& outJointEntities
    );

    std::vector<float> generateHeightmap(
        platypus::AssetManager* pAssetManager,
        const std::string& filepath,
        float heightMultiplier
    );
};
