#include "TerrainTestScene.hpp"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
#include <cmath>


using namespace platypus;


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
    pDirLight->direction = { 0.75f, -1.5f, 1.0f };

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );
    ImageFormat texImageFormat = ImageFormat::R8G8B8A8_SRGB;
    Texture* pBlendmapTexture = pAssetManager->loadTexture(
        "assets/textures/terrain/Blendmap.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );
    Texture* pDiffuseChannel0Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/ground_dry2_d.png",
        texImageFormat,
        textureSampler
    );
    Texture* pDiffuseChannel1Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/grass_ground_d.png",
        texImageFormat,
        textureSampler
    );
    Texture* pDiffuseChannel2Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/grass_rocky_d.png",
        texImageFormat,
        textureSampler
    );
    Texture* pDiffuseChannel3Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/ground_mud_d.png",
        texImageFormat,
        textureSampler
    );
    Texture* pDiffuseChannel4Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/jungle_mntn2_d.png",
        texImageFormat,
        textureSampler
    );
    Texture* pSpecularChannel4Texture = pAssetManager->loadTexture(
        "assets/textures/terrain/jungle_mntn2_s.png",
        texImageFormat,
        textureSampler
    );

    ID_t zeroTextureID = pAssetManager->getZeroTexture()->getID();
    ID_t blackTextureID = pAssetManager->getBlackTexture()->getID();
    ID_t whiteTextureID = pAssetManager->getWhiteTexture()->getID();

    Material* pTerrainMaterial = pAssetManager->createMaterial(
        MaterialType::TERRAIN,
        pBlendmapTexture->getID(),
        {
            pDiffuseChannel0Texture->getID(),
            pDiffuseChannel1Texture->getID(),
            pDiffuseChannel2Texture->getID(),
            pDiffuseChannel3Texture->getID(),
            pDiffuseChannel4Texture->getID(),
        },
        {
            blackTextureID,
            blackTextureID,
            blackTextureID,
            blackTextureID,
            pSpecularChannel4Texture->getID()
        },
        {
        }
    );

    size_t heightmapWidth = 32;
    size_t heightmapArea = heightmapWidth * heightmapWidth;
    _heightmap1.resize(heightmapArea);
    _heightmap2.resize(heightmapArea);
    float heightModifier = 0.003f;
    float tileWidth = 2.0f;
    float totalTerrainWidth = heightmapWidth * tileWidth;
    for (size_t z = 0; z < 2; ++z)
    {
        for (size_t x = 0; x < 2; ++x)
        {
            for (size_t i = 0; i < heightmapArea; ++i)
            {
                _heightmap1[i] = (float)(((int)std::rand() % 256) - 127) * heightModifier;
                _heightmap2[i] = (float)(((int)std::rand() % 256) - 127) * heightModifier;
            }
            TerrainMesh* pTerrainMesh = pAssetManager->createTerrainMesh(tileWidth, _heightmap1, false);

            entityID_t terrainEntity = createEntity();
            create_transform(
                terrainEntity,
                { x * totalTerrainWidth, 0, z * totalTerrainWidth },
                { { 0, 1, 0}, 0 },
                { 1, 1, 1 }
            );
            create_terrain_mesh_renderable(terrainEntity, pTerrainMesh->getID(), pTerrainMaterial->getID());
        }
    }
}


static float s_time = 0.0f;
void TerrainTestScene::update()
{
    _camController.update();

    /*
    float interpolationAmount = (std::sin(s_time) + 1.0f) * 0.5f;
    Debug::log("___TEST___amount: " + std::to_string(interpolationAmount));
    for (size_t i = 0; i < _heightmap1.size(); ++i)
    {
        float h1 = _heightmap1[i];
        float h2 = _heightmap2[i];
        float interpolatedHeight = h1 + ((h2 - h1) * interpolationAmount);
        size_t stride = sizeof(Vector3f) * 2;

        Buffer* pVertexBuffer = _pTerrainMesh->getVertexBuffer();
        pVertexBuffer->updateDevice(
            &interpolatedHeight,
            sizeof(float),
            i * stride + sizeof(float)
        );
    }

    s_time += 1.0f * Timing::get_delta_time();
    */
}
