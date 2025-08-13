#include "SkinnedMeshTestScene.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
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

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
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

    Model* pAnimatedModel = pAssetManager->loadModel(
        "assets/models/SkeletonTest3.glb",
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

    Texture* pDiffuseTexture = pAssetManager->loadTexture(
        "assets/textures/characterTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pBoxDiffuseTexture = pAssetManager->loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    Material* pMaterial = pAssetManager->createMaterial(
        pDiffuseTexture->getID(),
        pAssetManager->getWhiteTexture()->getID(),
        NULL_ID,
        0.8f,
        16.0f
    );
    Material* pBoxMaterial = pAssetManager->createMaterial(
        pBoxDiffuseTexture->getID(),
        pAssetManager->getWhiteTexture()->getID(),
        NULL_ID,
        0.8f,
        16.0f
    );
    Model* pModel = pAssetManager->loadModel("assets/TestCube.glb");

    const Pose& bindPose = bindPoses[0];
    entityID_t rootJointEntity = create_skeleton(
        bindPose.joints,
        bindPose.jointChildMapping
    );
    // Create the skinned mesh renderable
    //entityID_t skinnedEntity = createEntity();
    //create_transform(skinnedEntity, Matrix4f(1.0f));
    create_skinned_mesh_renderable(
        rootJointEntity,
        pAnimatedMesh->getID(),
        pMaterial->getID()
    );

    // Test putting box renderable on some skeleton hierarchy's transforms
    Children* pRootJointChildren = (Children*)getComponent(
        rootJointEntity,
        ComponentType::COMPONENT_TYPE_CHILDREN
    );
    create_static_mesh_renderable(
        pRootJointChildren->entityIDs[0],
        pModel->getMeshes()[0]->getID(),
        pBoxMaterial->getID()
    );

    SkeletalAnimationData* pAnimationAsset = pAssetManager->createSkeletalAnimation(
        1.0f,
        bindPoses[0],
        animations[0]
    );

    SkeletalAnimation* pAnimationComponent = create_skeletal_animation(
        rootJointEntity,
        pAnimationAsset->getID(),
        1.0f
    );

    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 0.75f, -1.5f, 1.0f };
}

void SkinnedMeshTestScene::update()
{
    _camController.update();
}
