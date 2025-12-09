#include "WaterTestScene.hpp"
#include <cmath>

using namespace platypus;


WaterTestScene::WaterTestScene() {}
WaterTestScene::~WaterTestScene() {}

void WaterTestScene::init()
{
    Debug::log("___TEST___init WaterTestScene");
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

    float terrainTileScale = 2.0f;
    std::vector<float> heightmap = generateHeightmap(pAssetManager, "assets/textures/Heightmap.png", 10.0f);
    int terrainTilesPerRow = sqrt(heightmap.size());
    Mesh* pTerrainMesh = pAssetManager->createTerrainMesh(terrainTileScale, heightmap, true, true);

    entityID_t terrainEntity = createEntity();
    create_transform(
        terrainEntity,
        { 0, 0, 0 },
        { { 0, 1, 0}, 0 },
        { 1, 1, 1 }
    );


    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );
    ImageFormat texImageFormat = ImageFormat::R8G8B8A8_SRGB;
    Texture* pBlendmapTexture = pAssetManager->loadTexture(
        "assets/textures/terrain/Blendmap2.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );
    std::vector<std::string> diffuseTexturePaths = {
        "assets/textures/terrain/ground_dry2_d.png",
        "assets/textures/terrain/grass_ground_d.png",
        "",
        "",
        ""
    };
    std::vector<std::string> specularTexturePaths = {
        "assets/textures/terrain/ground_dry2_s.png",
        "assets/textures/terrain/grass_ground_s.png",
        "",
        "",
        ""
    };
    std::vector<std::string> normalTexturePaths = {
        "assets/textures/terrain/ground_dry2_n.png",
        "assets/textures/terrain/grass_ground_n.png",
        "",
        "",
        ""
    };

    std::vector<ID_t> diffuseTextures = loadTextures(
        pAssetManager,
        texImageFormat,
        textureSampler,
        diffuseTexturePaths
    );
    std::vector<ID_t> specularTextures = loadTextures(
        pAssetManager,
        texImageFormat,
        textureSampler,
        specularTexturePaths
    );
    std::vector<ID_t> normalTextures = loadTextures(
        pAssetManager,
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler,
        normalTexturePaths
    );

    Material* pTerrainMaterial = pAssetManager->createMaterial(
        pBlendmapTexture->getID(),
        diffuseTextures,
        specularTextures,
        normalTextures,
        0.625f,
        32.0f,
        { 0, 0 },
        { (float)terrainTilesPerRow, (float)terrainTilesPerRow },
        false, // cast shadows
        false // receive shadows
    );

    create_renderable3D(terrainEntity, pTerrainMesh->getID(), pTerrainMaterial->getID());


    // Create water plane
    float s = 0.5f;
    std::vector<float> planeVertexData = {
        -s, 0.0f, -s,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -s, 0.0f, s,      0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        s, 0.0f, s,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        s, 0.0f, -s,      0.0f, 1.0f, 0.0f,   1.0f, 1.0f
    };
    std::vector<uint32_t> planeIndices = {
        0, 1, 2,
        2, 3, 0
    };
    VertexBufferLayout planeVertexBufferLayout(
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float3 },
            { 2, ShaderDataType::Float2 }
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    );
    Mesh* pPlaneMesh = pAssetManager->createMesh(
        planeVertexBufferLayout,
        planeVertexData,
        planeIndices,
        MeshType::MESH_TYPE_STATIC
    );

    Texture* pWaterTexture = pAssetManager->loadTexture(
        "assets/textures/Water.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pWaterDistortionTexture = pAssetManager->loadTexture(
        "assets/textures/DistortionMap.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    Material* pWaterMaterial = pAssetManager->createMaterial(
        NULL_ID,
        { pWaterTexture->getID() },
        { pWaterDistortionTexture->getID() },
        { },
        0.625f,
        16.0f,
        { 0, 0 },
        { 1, 1 },
        false, // cast shadows
        false, // receive shadows,
        "water/VertexShader",
        "water/FragmentShader"
    );

    float waterTextureScale = 2.0f;
    pWaterMaterial->setTextureProperties(
        { 0, 0 },
        { waterTextureScale, waterTextureScale }
    );

    entityID_t waterPlaneEntity = createEntity();
    float waterScale = (float)terrainTilesPerRow * terrainTileScale;
    float waterPosX = terrainTileScale * (terrainTilesPerRow * 0.5f);
    float waterPosY = 2.5f;
    float waterPosZ = terrainTileScale * (terrainTilesPerRow * 0.5f);
    create_transform(
        waterPlaneEntity,
        { waterPosX, waterPosY, waterPosZ },
        { { 0, 1, 0 }, 0.0f },
        { waterScale, waterScale, waterScale }
    );
    create_renderable3D(
        waterPlaneEntity,
        pPlaneMesh->getID(),
        pWaterMaterial->getID()
    );
}

void WaterTestScene::update()
{
    _camController.update();
}
