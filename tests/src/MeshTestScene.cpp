#include "MeshTestScene.h"

using namespace platypus;


MeshTestScene::MeshTestScene()
{
}

MeshTestScene::~MeshTestScene()
{
}

void MeshTestScene::init()
{
    initBase();

    _camController.init();
    _camController.set(
        0, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 0, 0});

    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        1,
        0
    );

    Texture* pTexture = assetManager.loadTexture(
        "assets/textures/DiffuseTest.png",
        textureSampler
    );

    Material* pMaterial = assetManager.createMaterial(
        pTexture->getID(),
        assetManager.getWhiteTexture()->getID()
    );

    Model* pModel = assetManager.loadModel("assets/models/MultiTest2.glb");

    for (Mesh* pMesh : pModel->getMeshes())
    {
        entityID_t entity = createEntity();
        Transform* pTransform = create_transform(
            entity,
            pMesh->getTransformationMatrix()
        );

        pTransform->globalMatrix[2 + 3 * 4] = -12.0f;

        StaticMeshRenderable* pRenderable = create_static_mesh_renderable(
            entity,
            pMesh->getID(),
            pMaterial->getID()
        );
    }
}


void MeshTestScene::update()
{
    updateBase();

    Transform* pCamTransform = (Transform*)getComponent(
        _cameraEntity,
        ComponentType::COMPONENT_TYPE_TRANSFORM
    );
    _camController.update(pCamTransform);
}
