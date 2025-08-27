#include "SkinnedMeshTestScene.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
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
    Model* pAnimatedModel = pAssetManager->loadModel(
        "assets/models/SkeletonTest3.glb",
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

    std::vector<Pose> pistolBindPoses;
    std::vector<std::vector<Pose>> pistolAnimations;
    Model* pPistolModel = pAssetManager->loadModel(
        "assets/models/Pistol.glb",
        pistolBindPoses,
        pistolAnimations
    );
    Mesh* pPistolMesh = pPistolModel->getMeshes()[0];
    SkeletalAnimationData* pPistolAnimationAsset = pAssetManager->createSkeletalAnimation(
        1.0f, // NOTE: seems that asset's speed isn't used but the component's speed instead
        pistolBindPoses[0],
        pistolAnimations[0]
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
    Texture* pPistolTexture = pAssetManager->loadTexture(
        "assets/textures/Pistol.png",
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
    Material* pPistolMaterial = pAssetManager->createMaterial(
        pPistolTexture->getID(),
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


    int area = 11;
    float spacing = 3.25f;
    /*
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
    */

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
                pMaterial,
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

            float pistolScale = 0.125f;
            std::vector<entityID_t> pistolJointEntities;
            entityID_t pistolEntity = create_animated_entity(
                this,
                pPistolMesh,
                pistolBindPoses[0],
                pPistolAnimationAsset,
                pPistolMaterial,
                { 0, 0.15f, 0 },
                { { 1, 0, 0}, PLATY_MATH_PI * -0.5f },
                { pistolScale, pistolScale, pistolScale },
                pistolJointEntities
            );

            glue_to_joint(
                pistolEntity,
                _jointEntities,
                bindPoses[0],
                "hand1"
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

static bool s_tabDown = false;
static bool s_backspaceDown = false;
void SkinnedMeshTestScene::update()
{
    _camController.update();

    InputManager& inputManager = Application::get_instance()->getInputManager();

    // Test removing entities, especially from the middle of the hierarchy
    /*
    if (inputManager.isKeyDown(KeyName::KEY_TAB) && !s_tabDown)
    {
        s_tabDown = true;
        _selectedJointIndex = (_selectedJointIndex + 1) % _bindPose.joints.size();
        _selectedJointEntity = _jointEntities[_selectedJointIndex];
        std::string selectedJointName = get_joint_name(_bindPose, _selectedJointIndex);
        Debug::log(
            "___TEST___Selected joint: " + selectedJointName
        );
    }
    if (!inputManager.isKeyDown(KeyName::KEY_TAB) && s_tabDown)
        s_tabDown = false;


    if (inputManager.isKeyDown(KeyName::KEY_BACKSPACE) && !s_backspaceDown)
    {
        s_backspaceDown = true;

        Debug::log(
            "___TEST___deleting joint: " + get_joint_name(_bindPose, _selectedJointIndex)
        );
        destroyEntity(_selectedJointEntity);
    }
    if (!inputManager.isKeyDown(KeyName::KEY_BACKSPACE) && s_backspaceDown)
        s_backspaceDown = false;
    */
}
