#include "TestScene.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include "platypus/assets/Image.h"
#include "platypus/assets/Texture.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Timing.h"

#include <memory>


using namespace platypus;


TestScene::TestScene()
{
}

TestScene::~TestScene()
{
}

static Mesh* create_ground_mesh(AssetManager& assetManager, float scale)
{
    float t = scale * 0.5f;
    std::vector<float> vertexData = {
       -scale * 0.5f, 0, -scale * 0.5f, 0, 1, 0,  0, 0,
       -scale * 0.5f, 0,  scale * 0.5f, 0, 1, 0,  0, t,
        scale * 0.5f, 0,  scale * 0.5f, 0, 1, 0,  t, t,
        scale * 0.5f, 0, -scale * 0.5f, 0, 1, 0,  t, 0
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    return assetManager.createMesh(
        vertexData,
        indices
    );
}

static Mesh* create_grass_mesh(AssetManager& assetManager, float scale)
{
    float s = scale * 0.5f;
    std::vector<float> vertexData = {
       -s, scale, 0,    0, 1, 0,    0, 0,
       -s,  0, 0,       0, 1, 0,    0, 1,
        s,  0, 0,       0, 1, 0,    1, 1,
        s, scale, 0,    0, 1, 0,    1, 0,

       0, scale, -s,    0, 1, 0,    0, 0,
       0,  0,    -s,       0, 1, 0,    0, 1,
       0,  0,     s,       0, 1, 0,    1, 1,
       0, scale,  s,    0, 1, 0,    1, 0
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4
    };

    return assetManager.createMesh(
        vertexData,
        indices
    );
}

void TestScene::init()
{
    float clearBrightness = 0.5f;
    environmentProperties.clearColor = { clearBrightness, clearBrightness, clearBrightness, 1.0f };

    _camEntity = createEntity();
    createTransform(
        _camEntity,
        { 0.0f, 0.0f, 0.0f },
        { {0, 1, 0}, 0.0f },
        { 1, 1, 1 }
    );
    Matrix4f camProjMat = create_perspective_projection_matrix(
        800.0f / 600.0f,
        1.3f,
        0.1f,
        100.0f
    );
    Camera* pCamera = createCamera(_camEntity, camProjMat);
    _camController.init();

    _camController.set(
        PLATY_MATH_PI * 0.25f, // pitch
        0.0f, // yaw
        0.0025f, // rot speed
        40.0f, // zoom
        80.0f, // max zoom
        1.25f // zoom speed
    );

    entityID_t dirLightEntity = createEntity();
    createDirectionalLight(dirLightEntity, { 0.5f, -0.5f, -0.5f }, { 1, 1, 1 });


    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    // Create plane, representing floor
    entityID_t groundEntity = createEntity();
    createTransform(
        groundEntity,
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1 }
    );

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        1,
        0
    );
    Image* pTerrainGrassImage = assetManager.loadImage("assets/textures/TerrainGrass.png");
    Texture* pTerrainGrassTexture = assetManager.createTexture(pTerrainGrassImage->getID(), textureSampler);

    float areaScale = 60.0f;
    float halfAreaScale = areaScale * 0.5f;
    Mesh* pGroundMesh = create_ground_mesh(assetManager, areaScale);

    createStaticMeshRenderable(
        groundEntity,
        pGroundMesh->getID(),
        pTerrainGrassTexture->getID()
    );

    // Create some grass entities
    Image* pGrassImage = assetManager.loadImage("assets/textures/Grass.png");
    Texture* pGrassTexture = assetManager.createTexture(pGrassImage->getID(), textureSampler);
    Mesh* pGrassMesh = create_grass_mesh(assetManager, 1.25f);
    size_t grassCount = 100;
    for (size_t i = 0; i < grassCount; ++i)
    {
        entityID_t entity = createEntity();
        float randX = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);
        float randZ = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);

        float randRot = (float)(std::rand() % 180);
        randRot = randRot / (float)(PLATY_MATH_PI * 2.0);
        createTransform(
            entity,
            { randX, 0, randZ },
            { {0, 1, 0}, randRot },
            { 1, 1, 1 }
        );

        createStaticMeshRenderable(
            entity,
            pGrassMesh->getID(),
            pGrassTexture->getID()
        );
    }


    // Create tree entities
    Model* pTreeModel1 = assetManager.loadModel("assets/models/FirTree.glb");
    Model* pTreeModel2 = assetManager.loadModel("assets/models/PineTree.glb");
    Image* pTreeTextureImage = assetManager.loadImage("assets/textures/FirTreeTexture.png");
    Texture* pFirTreeTexture = assetManager.createTexture(pTreeTextureImage->getID(), textureSampler);
    Texture* pPineTreeTexture = assetManager.createTexture(pTreeTextureImage->getID(), textureSampler);

    size_t treeCount = 51;
    for (size_t i = 0; i < treeCount; ++i)
    {
        entityID_t entity = createEntity();
        float randX = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);
        float randZ = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);
        createTransform(
            entity,
            { randX, 0, randZ },
            { {0, 1, 0}, 0.0f },
            { 0.5f, 0.5f, 0.5f }
        );

        ID_t useMeshID = pTreeModel1->getMeshes()[0]->getID();
        ID_t useTextureID = pFirTreeTexture->getID();
        int randType = std::rand() % 4;
        if (randType == 3)
        {
            useMeshID = pTreeModel2->getMeshes()[0]->getID();
            useTextureID = pPineTreeTexture->getID();
        }

        createStaticMeshRenderable(
            entity,
            useMeshID,
            useTextureID
        );
    }
}

static float s_TEST_value = 0.0f;
void TestScene::update()
{
    /*
    s_TEST_value += 1.0f * Timing::get_delta_time();
    Matrix4f newMatrix = create_transformation_matrix(
        { 0.0f, 1.0f, -5.0f },
        { { 0, 1, 1 }, s_TEST_value },
        { 1, 1, 1 }
    );
    Transform* pTransform = (Transform*)getComponent(
        testEntity2,
        ComponentType::COMPONENT_TYPE_TRANSFORM
    );
    pTransform->globalMatrix = newMatrix;
    */

    Transform* pCamTransform = (Transform*)getComponent(_camEntity, ComponentType::COMPONENT_TYPE_TRANSFORM);
    _camController.update(pCamTransform);
}
