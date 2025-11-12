#include "ShadowTestScene.hpp"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"


using namespace platypus;

static std::vector<ID_t> load_textures(
    AssetManager* pAssetManager,
    ImageFormat imageFormat,
    TextureSampler sampler,
    std::vector<std::string> filepaths
)
{
    std::vector<ID_t> textures;
    for (const std::string& path : filepaths)
    {
        textures.push_back(
            pAssetManager->loadTexture(
                path,
                imageFormat,
                sampler
            )->getID()
        );
    }
    return textures;
}

ShadowTestScene::ShadowTestScene()
{
}

ShadowTestScene::~ShadowTestScene()
{
}

void ShadowTestScene::init()
{
    Debug::log("___TEST___init ShadowTestScene");
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

    Light* pDirLight = (Light*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_LIGHT
    );
    pDirLight->type = LightType::DIRECTIONAL_LIGHT;
    pDirLight->direction = { 1.0f, -1.0f, 0.0f };
    pDirLight->direction = pDirLight->direction.normalize();
    pDirLight->enableShadows = true;
    pDirLight->maxShadowDistance = 30.0f;

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );

    size_t heightmapWidth = 32;
    size_t tilesPerRow = heightmapWidth - 1;
    size_t heightmapArea = heightmapWidth * heightmapWidth;
    std::vector<float> heightmap(heightmapArea);
    float heightModifier = 0.003f;
    for (size_t i = 0; i < heightmapArea; ++i)
    {
        heightmap[i] = (float)(((int)std::rand() % 256) - 127) * heightModifier;
    }
    _pTerrainMesh = pAssetManager->createTerrainMesh(2.0f, heightmap, true, true);

    entityID_t terrainEntity = createEntity();
    create_transform(
        terrainEntity,
        { 0, 0, 0 },
        { { 0, 1, 0}, 0 },
        { 1, 1, 1 }
    );


    ImageFormat texImageFormat = ImageFormat::R8G8B8A8_SRGB;
    Texture* pBlendmapTexture = pAssetManager->loadTexture(
        "assets/textures/terrain/Blendmap.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );
    std::vector<std::string> diffuseTexturePaths = {
        "assets/textures/terrain/ground_dry2_d.png",
        "assets/textures/terrain/grass_ground_d.png",
        "assets/textures/terrain/grass_rocky_d.png",
        "assets/textures/terrain/ground_mud_d.png",
        "assets/textures/terrain/jungle_mntn2_d.png"
    };
    std::vector<std::string> specularTexturePaths = {
        "assets/textures/terrain/ground_dry2_s.png",
        "assets/textures/terrain/grass_ground_s.png",
        "assets/textures/terrain/grass_rocky_s.png",
        "assets/textures/terrain/ground_mud_s.png",
        "assets/textures/terrain/jungle_mntn2_s.png"
    };
    std::vector<std::string> normalTexturePaths = {
        "assets/textures/terrain/ground_dry2_n.png",
        "assets/textures/terrain/grass_ground_n.png",
        "assets/textures/terrain/grass_rocky_n.png",
        "assets/textures/terrain/ground_mud_n.png",
        "assets/textures/terrain/jungle_mntn2_n.png"
    };

    std::vector<ID_t> diffuseTextures = load_textures(
        pAssetManager,
        texImageFormat,
        textureSampler,
        diffuseTexturePaths
    );
    std::vector<ID_t> specularTextures = load_textures(
        pAssetManager,
        texImageFormat,
        textureSampler,
        specularTexturePaths
    );
    std::vector<ID_t> normalTextures = load_textures(
        pAssetManager,
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler,
        normalTexturePaths
    );

    _pTerrainMaterial = pAssetManager->createMaterial(
        MaterialType::TERRAIN,
        pBlendmapTexture->getID(),
        diffuseTextures,
        specularTextures,
        normalTextures,
        0.625f,
        32.0f,
        { 0, 0 },
        { (float)tilesPerRow, (float)tilesPerRow },
        true, // receive shadows
        false
    );

    create_terrain_mesh_renderable(terrainEntity, _pTerrainMesh->getID(), _pTerrainMaterial->getID());

    // Test meshes that cast shadows
    Material* pStaticMeshMaterial = createMeshMaterial(
        pAssetManager,
        "assets/textures/DiffuseTest.png",
        true,
        true
    );
    Material* pSkinnedMeshMaterial = createMeshMaterial(
        pAssetManager,
        "assets/textures/characterTest.png",
        false,
        true
    );

    Mesh* pStaticMesh = pAssetManager->loadModel("assets/TestCube.glb")->getMeshes()[0];
    entityID_t boxEntity = createStaticMeshEntity(
        { 15, 0, 11 },
        { { 0, 1, 0 }, 0.0f },
        { 1, 1, 1 },
        pStaticMesh->getID(),
        pStaticMeshMaterial->getID()
    );

    entityID_t boxEntity2 = createStaticMeshEntity(
        { 10, 0, 10 },
        { { 0, 2, 0 }, 0.0f },
        { 2, 2, 2 },
        pStaticMesh->getID(),
        pStaticMeshMaterial->getID()
    );

    std::vector<KeyframeAnimationData> animations;
    Model* pAnimatedModel = pAssetManager->loadModel(
        "assets/models/MultiAnimSkeletonTest.glb",
        animations
    );
    Mesh* pSkinnedMesh = pAnimatedModel->getMeshes()[0];
    SkeletalAnimationData* pAnimationAsset1 = pAssetManager->createSkeletalAnimation(animations[0]);
    SkeletalAnimationData* pAnimationAsset2 = pAssetManager->createSkeletalAnimation(animations[1]);

    int area = 2;
    float spacing = 3.0f;
    float startX = 20.0f;
    float startZ = 20.0f;
    for (int z = 0; z < area; ++z)
    {
        for (int x = 0; x < area; ++x)
        {
            std::vector<entityID_t> jointEntities;
            entityID_t animatedEntity = createSkinnedMeshEntity(
                { startX + x * spacing, 0, startZ + z * spacing },
                { { 0, 1,0 }, 0 },
                { 1, 1, 1 },
                pSkinnedMesh,
                pAnimationAsset1,
                pSkinnedMeshMaterial->getID(),
                jointEntities
            );
            SkeletalAnimation* pAnim = (SkeletalAnimation*)getComponent(
                jointEntities[0],
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );
            pAnim->mode = AnimationMode::ANIMATION_MODE_PLAY_ONCE;
        }
    }

    // For debugging framebuffers
    TextureSampler framebufferDebugTextureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        false,
        0
    );
    _pTestTexture1 = pAssetManager->loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        framebufferDebugTextureSampler
    );
    _pTestTexture2 = pAssetManager->loadTexture(
        "assets/textures/Floor.png",
        ImageFormat::R8G8B8A8_SRGB,
        framebufferDebugTextureSampler
    );

    _framebufferDebugEntity = createEntity();
    create_gui_transform(
        _framebufferDebugEntity,
        { 0, 0 },
        { 400, 256 }
    );
    MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
    GUIRenderable* pFramebufferDebugRenderable = create_gui_renderable(
        _framebufferDebugEntity,
        pMasterRenderer->getShadowFramebufferDepthTexture()->getID()
    );
}

void ShadowTestScene::update()
{
    _camController.update();

    Application* pApp = Application::get_instance();
    InputManager& inputManager = pApp->getInputManager();

    // Test setting dir light's shadow proj and view matrices to match camera when pressing E
    if (inputManager.isKeyDown(KeyName::KEY_E))
    {
        Light* pDirLight = (Light*)getComponent(
            _lightEntity,
            ComponentType::COMPONENT_TYPE_LIGHT
        );
        Camera* pCamera = (Camera*)getComponent(
            _cameraEntity,
            ComponentType::COMPONENT_TYPE_CAMERA
        );
        Transform* pCameraTransform = (Transform*)getComponent(
            _cameraEntity,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        //pDirLight->shadowProjectionMatrix = pCamera->perspectiveProjectionMatrix;
        //Matrix4f viewMatrix = pCameraTransform->globalMatrix.inverse();
        //pDirLight->shadowViewMatrix = viewMatrix;
        pDirLight->direction = get_transform_forward(pCameraTransform);
    }

    Light* pDirLight = (Light*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_LIGHT
    );

    const float distChangeSpeed = 12.0f;
    if (inputManager.isKeyDown(KeyName::KEY_1))
    {
        if (pDirLight->maxShadowDistance > 5)
        {
            pDirLight->maxShadowDistance -= distChangeSpeed * Timing::get_delta_time();
            Debug::log("___TEST___MAX SHADOW DISTANCE = " + std::to_string(pDirLight->maxShadowDistance));
        }
    }
    if (inputManager.isKeyDown(KeyName::KEY_2))
    {
        if (pDirLight->maxShadowDistance < 2000)
        {
            pDirLight->maxShadowDistance += distChangeSpeed * Timing::get_delta_time();
            Debug::log("___TEST___MAX SHADOW DISTANCE = " + std::to_string(pDirLight->maxShadowDistance));
        }
    }

    GUIRenderable* pFramebufferDebugRenderable = (GUIRenderable*)getComponent(
        _framebufferDebugEntity,
        ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
    );

    MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
    Texture* framebufferTexture = pMasterRenderer->getShadowFramebufferDepthTexture();
    if (framebufferTexture)
    {
        pFramebufferDebugRenderable->textureID = framebufferTexture->getID();
    }
    else
    {
        pFramebufferDebugRenderable->textureID = pApp->getAssetManager()->getWhiteTexture()->getID();
    }
}
