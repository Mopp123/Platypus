#include "SkinnedMeshTestScene.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
#include "MaterialTestScene.h"
#include <string>


using namespace platypus;

static entityID_t create_animated_entity(
    Scene* pScene,
    Mesh* pMesh,
    const Pose& bindPose,
    SkeletalAnimationData* pAnimationAsset,
    Material* pMaterial,
    Vector3f position,
    Quaternion rotation,
    Vector3f scale,
    std::vector<entityID_t>& outJointEntities
)
{
    // Create entity containing the skeleton
    //  -> otherwise root joint doesn't get animated at all
    entityID_t entity = pScene->createEntity();
    create_transform(
        entity,
        position,
        rotation,
        scale
    );

    outJointEntities = create_skeleton(
        bindPose.joints,
        bindPose.jointChildMapping
    );
    entityID_t rootJointEntity = outJointEntities[0];
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


static void glue_to_joint(
    entityID_t entity,
    std::vector<entityID_t>& jointEntities,
    Pose& bindPose,
    std::string jointName
)
{
    // Find correct joint index
    int jointIndex = 0;
    bool found = false;
    for (int i = 0; i < bindPose.joints.size(); ++i)
    {
        if (bindPose.joints[i].name == jointName)
        {
            jointIndex = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        Debug::log(
            "@create_joint_box "
            "No joint found with name: " + jointName,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }
    entityID_t jointEntity = jointEntities[jointIndex];

    add_child(jointEntity, entity);
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
    // NOTE:
    // File: "assets/models/SkeletonTest3Linear2.glb" works because it
    // has keyframes for ALL joints at the SAME TIME for EVERY KEYFRAME!
    Model* pAnimatedModel = pAssetManager->loadModel(
        "assets/models/SkeletonTest3Linear2.glb",
        bindPoses,
        animations
    );
    _bindPose = bindPoses[0];
    Mesh* pAnimatedMesh = pAnimatedModel->getMeshes()[0];
    SkeletalAnimationData* pAnimationAsset = pAssetManager->createSkeletalAnimation(
        1.0f, // NOTE: seems that asset's speed isn't used but the component's speed instead
        bindPoses[0],
        animations[0]
    );

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


    int area = 1;
    float spacing = 3.25f;

    for (int x = 0; x < area; ++x)
    {
        for (int z = 0; z < area; ++z)
        {
            float animatedEntityScale = 1.0f;
            entityID_t animatedEntity = create_animated_entity(
                this,
                pAnimatedMesh,
                bindPoses[0],
                pAnimationAsset,
                pBoxMaterial,
                { x * spacing, 0, -z * spacing },
                { { 0, 1, 0}, 0.0f },
                { animatedEntityScale, animatedEntityScale, animatedEntityScale },
                _jointEntities
            );
            /*
            const float cubeScale = 0.3f;
            entityID_t boxEntity = createEntity();
            create_transform(
                boxEntity,
                { 0, 0, 0 },
                { { 0, 1, 0}, 0.0f },
                { cubeScale, cubeScale, cubeScale }
            );
            create_static_mesh_renderable(
                boxEntity,
                pBoxModel->getMeshes()[0]->getID(),
                pBoxMaterial->getID()
            );

            glue_to_joint(
                boxEntity,
                _jointEntities,
                bindPoses[0],
                "hand0"
            );
            */
        }
    }

    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 0.75f, -1.5f, 1.0f };
}

static std::string get_joint_name(const Pose& bindPose, size_t index)
{
    if (index >= bindPose.joints.size())
    {
        Debug::log(
            "@print_joint_name "
            "Joint index (" + std::to_string(index) + ") "
            "joint count is " + std::to_string(bindPose.joints.size()),
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }
    return bindPose.joints[index].name;
}

void SkinnedMeshTestScene::update()
{
    _camController.update();

    InputManager& inputManager = Application::get_instance()->getInputManager();
    if (inputManager.isKeyDown(KeyName::KEY_0))
        Application::get_instance()->getSceneManager().assignNextScene(new MaterialTestScene);
}
