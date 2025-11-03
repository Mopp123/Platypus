#include "OpenglUpgradeTestScene.hpp"

using namespace platypus;


OpenglUpgradeTestScene::OpenglUpgradeTestScene()
{
}

OpenglUpgradeTestScene::~OpenglUpgradeTestScene()
{
}

void OpenglUpgradeTestScene::init()
{
    initBase();

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();

    InputManager& inputManager = Application::get_instance()->getInputManager();

    _camController.init(_cameraEntity);
    _camController.set(
        0, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 0, 0});

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );

    Texture* pDiffuseTexture = pAssetManager->loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pSpecularTexture = pAssetManager->loadTexture(
        "assets/textures/SpecularTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pNormalTexture = pAssetManager->loadTexture(
        "assets/textures/NormalTest.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );

    Material* pMaterial = pAssetManager->createMaterial(
        MaterialType::MESH,
        NULL_ID,
        { pDiffuseTexture->getID() },
        { pSpecularTexture->getID() },
        { pNormalTexture->getID() },
        0.8f,
        16.0f
    );

    Model* pModel = pAssetManager->loadModel("assets/TestCubeTangents.glb");

    createStaticMeshEntity(
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0.0f},
        { 1, 1, 1 },
        pModel->getMeshes()[0]->getID(),
        pMaterial->getID()
    );


    /*
    Material* pSkinnedMeshMaterial = createMeshMaterial(
        pAssetManager,
        "assets/textures/characterTest.png",
        false
    );

    std::vector<KeyframeAnimationData> animations;
    Model* pAnimatedModel = pAssetManager->loadModel(
        "assets/models/MultiAnimSkeletonTest.glb",
        animations
    );
    Mesh* pSkinnedMesh = pAnimatedModel->getMeshes()[0];
    SkeletalAnimationData* pAnimationAsset = pAssetManager->createSkeletalAnimation(animations[0]);

    std::vector<entityID_t> jointEntities;
    entityID_t animatedEntity = createSkinnedMeshEntity(
        { 10, 0, 0 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1 },
        pSkinnedMesh,
        pAnimationAsset,
        pSkinnedMeshMaterial->getID(),
        jointEntities
    );
    */

    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 0.75f, -1.5f, -1.0f };
}

void OpenglUpgradeTestScene::update()
{
    updateBase();
    _camController.update();
}
