#include "TerrainTestScene.hpp"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"


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
    Texture* pDiffuseTexture = pAssetManager->loadTexture(
        "assets/textures/TerrainGrass.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    TerrainMaterial* pTerrainMaterial = pAssetManager->createTerrainMaterial(
        pDiffuseTexture->getID()
    );

    size_t heightmapWidth = 32;
    size_t heightmapArea = heightmapWidth * heightmapWidth;
    std::vector<float> heightmap(heightmapArea);
    float heightModifier = 0.01f;
    for (float& h : heightmap)
    {
        h = (float)(std::rand() % 256) * heightModifier;
    }
    TerrainMesh* pTerrainMesh = pAssetManager->createTerrainMesh(2.0f, heightmap, false);

    entityID_t terrainEntity = createEntity();
    create_transform(
        terrainEntity,
        { 0, 0, 0 },
        { { 0, 1, 0}, 0 },
        { 1, 1, 1 }
    );
    create_terrain_mesh_renderable(terrainEntity, pTerrainMesh->getID(), pTerrainMaterial->getID());

}

void TerrainTestScene::update()
{
    _camController.update();
}
