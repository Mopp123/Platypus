#include "SkinnedMeshTestScene.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
#include <string>

using namespace platypus;

entityID_t create_animated_entity(
    Scene* pScene,
    Mesh* pMesh,
    const Pose& bindPose,
    SkeletalAnimationData* pAnimationAsset,
    Material* pMaterial,
    Vector3f position
)
{
    // Create entity containing the skeleton
    //  -> otherwise root joint doesn't get animated at all
    entityID_t entity = pScene->createEntity();
    create_transform(
        entity,
        position,
        { { 0, 1, 0 }, 0.0f},
        { 1, 1, 1 }
    );

    std::vector<entityID_t> jointEntities = create_skeleton(
        bindPose.joints,
        bindPose.jointChildMapping
    );
    entityID_t rootJointEntity = jointEntities[0];
    create_skinned_mesh_renderable(
        rootJointEntity,
        pMesh->getID(),
        pMaterial->getID()
    );
    create_skeletal_animation(
        rootJointEntity,
        pAnimationAsset->getID(),
        4.0f
    );

    add_child(entity, rootJointEntity);

    return entity;
}


// For helping debugging joint transforms
void create_joint_boxes(
    Scene* pScene,
    Mesh* pBoxMesh,
    Material* pBoxMaterial,
    std::vector<entityID_t>& jointEntities
)
{
    const float cubeScale = 0.3f;
    for (size_t i = 0; i < jointEntities.size(); ++i)
    {
        entityID_t e = pScene->createEntity();
        create_transform(
            e,
            { 0, 0, 0 },
            { { 0, 1, 0}, 0.0f },
            { cubeScale, cubeScale, cubeScale }
        );
        add_child(jointEntities[i], e);
        create_static_mesh_renderable(
            e,
            pBoxMesh->getID(),
            pBoxMaterial->getID()
        );
    }
}


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
    Model* pBoxModel = pAssetManager->loadModel("assets/TestCube.glb");

    SkeletalAnimationData* pAnimationAsset = pAssetManager->createSkeletalAnimation(
        1.0f, // NOTE: seems that asset's speed isn't used but the component's speed instead
        bindPoses[0],
        animations[0]
    );

    int area = 10;
    float spacing = 3.0f;
    for (int x = 0; x < area; ++x)
    {
        for (int z = 0; z < area; ++z)
        {
            create_animated_entity(
                this,
                pAnimatedMesh,
                bindPoses[0],
                pAnimationAsset,
                pMaterial,
                { x * spacing, 0, -z * spacing }
            );
        }
    }

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
