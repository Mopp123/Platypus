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
            pTexture->getID()
        );
    }
}

void MeshTestScene::update()
{
    updateBase();
}
