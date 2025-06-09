#include "SkinnedMeshTestScene.h"
#include "platypus/ecs/components/Renderable.h"
#include <string>

using namespace platypus;


SkinnedMeshTestScene::SkinnedMeshTestScene()
{
}

SkinnedMeshTestScene::~SkinnedMeshTestScene()
{
}

void SkinnedMeshTestScene::init()
{
    initBase();

    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    InputManager& inputManager = Application::get_instance()->getInputManager();

    _camController.init(_cameraEntity);
    _camController.set(
        0, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        20.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 0, 0 });

    std::vector<Pose> bindPoses;
    std::vector<std::vector<Pose>> animations;

    Debug::log("___TEST___ATTEMPT LOAD ANIM MODEL");
    Model* pAnimatedModel = assetManager.loadModel(
        "assets/models/SkeletonTest.glb",
        bindPoses,
        animations
    );
    Mesh* pAnimatedMesh = pAnimatedModel->getMeshes()[0];

    // Create box entity for all skeleton model's joints
    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );

    Texture* pDiffuseTexture = assetManager.loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    Material* pMaterial = assetManager.createMaterial(
        pDiffuseTexture->getID(),
        assetManager.getWhiteTexture()->getID(),
        NULL_ID,
        0.8f,
        16.0f
    );
    Model* pModel = assetManager.loadModel("assets/TestCube.glb");

    const Pose& bindPose = bindPoses[0];
    std::vector<entityID_t> jointEntities = create_skeleton(
        bindPose.joints,
        bindPose.jointChildMapping
    );
    // Create renderables representing joints
    for (entityID_t entity : jointEntities)
    {
        create_static_mesh_renderable(
            entity,
            pModel->getMeshes()[0]->getID(),
            pMaterial->getID()
        );
    }
    SkeletalAnimationData* pAnimationAsset = assetManager.createSkeletalAnimation(
        1.0f,
        bindPoses[0],
        animations[0]
    );

    SkeletalAnimation* pAnimationComponent = create_skeletal_animation(
        jointEntities[0],
        pAnimationAsset->getID(),
        1.0f
    );


    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 0.75f, -1.5f, -1.0f };
}

void SkinnedMeshTestScene::update()
{
    _camController.update();
}
