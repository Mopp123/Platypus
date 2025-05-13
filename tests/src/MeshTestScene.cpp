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

    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        1,
        0
    );

    Image* pImage = assetManager.loadImage("assets/test.png");
    Texture* pTexture = assetManager.createTexture(
        pImage->getID(),
        textureSampler
    );
    Model* pModel = assetManager.loadModel("assets/TestCube.glb");

    entityID_t entity = createEntity();
    Transform* pTransform = create_transform(
        entity,
        { 0, -1.0f, -10 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1}
    );

    StaticMeshRenderable* pRenderable = create_static_mesh_renderable(
        entity,
        pModel->getMeshes()[0]->getID(),
        pTexture->getID()
    );
}

void MeshTestScene::update()
{
    updateBase();
}
