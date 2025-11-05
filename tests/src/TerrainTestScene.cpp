#include "TerrainTestScene.hpp"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
#include <cmath>


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

TerrainTestScene::TerrainTestScene()
{
}

TerrainTestScene::~TerrainTestScene()
{
}

void TerrainTestScene::init()
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

    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 1.0f, -1.0f, 0.0f };
    pDirLight->direction = pDirLight->direction.normalize();

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );

    size_t heightmapWidth = 32;
    size_t tilesPerRow = heightmapWidth - 1;
    size_t heightmapArea = heightmapWidth * heightmapWidth;
    _heightmap1.resize(heightmapArea);
    _heightmap2.resize(heightmapArea);
    float heightModifier = 0.003f;
    for (size_t i = 0; i < heightmapArea; ++i)
    {
        _heightmap1[i] = (float)(((int)std::rand() % 256) - 127) * heightModifier;
        _heightmap2[i] = (float)(((int)std::rand() % 256) - 127) * heightModifier;
    }
    _pTerrainMesh = pAssetManager->createTerrainMesh(2.0f, _heightmap1, true, true);

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
        false, // receive shadows
        false // shadeless
    );

    create_terrain_mesh_renderable(terrainEntity, _pTerrainMesh->getID(), _pTerrainMaterial->getID());

    _pMeshMaterial = createMeshMaterial(
        pAssetManager,
        "assets/textures/DiffuseTest.png",
        true
    );
    Mesh* pStaticMesh = pAssetManager->loadModel("assets/TestCube.glb")->getMeshes()[0];
    entityID_t boxEntity = createStaticMeshEntity(
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0.0f },
        { 1, 1, 1 },
        pStaticMesh->getID(),
        _pMeshMaterial->getID()
    );
}


static float s_time = 0.0f;
void TerrainTestScene::update()
{
    _camController.update();

    Application* pApp = Application::get_instance();
    InputManager& inputManager = pApp->getInputManager();

    /*
    Vector2f newOffset = _pTerrainMaterial->getTextureOffset();
    newOffset.x += Timing::get_delta_time();
    newOffset.y += Timing::get_delta_time();
    _pTerrainMaterial->setTextureProperties(newOffset, _pTerrainMaterial->getTextureScale());
    */

    /*
    float interpolationAmount = (std::sin(s_time) + 1.0f) * 0.5f;
    for (size_t i = 0; i < _heightmap1.size(); ++i)
    {
        float h1 = _heightmap1[i];
        float h2 = _heightmap2[i];
        float interpolatedHeight = h1 + ((h2 - h1) * interpolationAmount);
        size_t stride = sizeof(Vector3f) * 2 + sizeof(Vector2f) + sizeof(Vector4f);

        Buffer* pVertexBuffer = _pTerrainMesh->getVertexBuffer();
        pVertexBuffer->updateDevice(
            &interpolatedHeight,
            sizeof(float),
            i * stride + sizeof(float)
        );
    }
    */

    s_time += 1.0f * Timing::get_delta_time();
}
