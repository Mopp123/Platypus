#include "TestScene.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include "platypus/assets/Image.h"
#include "platypus/assets/Texture.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Timing.h"

#include <memory>


using namespace platypus;


void TestScene::SceneWindowResizeEvent::func(int w, int h)
{
    Camera* pCameraComponent = (Camera*)_pScene->getComponent(
        _cameraEntity,
        ComponentType::COMPONENT_TYPE_CAMERA
    );

    float useHeight = h > 0 ? (float)h : 1.0f;
    pCameraComponent->perspectiveProjectionMatrix = create_perspective_projection_matrix(
        (float)w / useHeight,
        _fov,
        _zNear,
        _zFar
    );
}

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
       0,  0,    -s,    0, 1, 0,    0, 1,
       0,  0,     s,    0, 1, 0,    1, 1,
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
    std::srand(23618);


    environmentProperties.clearColor = { 0.1f, 0.2f, 0.35f, 1.0f };

    _camEntity = createEntity();
    createTransform(
        _camEntity,
        { 0.0f, 0.0f, 0.0f },
        { {0, 1, 0}, 0.0f },
        { 1, 1, 1 }
    );
    int windowSurfaceWidth = 0;
    int windowSurfaceHeight = 0;
    Application::get_instance()->getWindow().getSurfaceExtent(
        &windowSurfaceWidth, &windowSurfaceHeight
    );
    float aspectRatio = 1.7f;
    if (windowSurfaceHeight > 0)
        aspectRatio = (float)windowSurfaceWidth / (float)windowSurfaceHeight;

    Matrix4f camProjMat = create_perspective_projection_matrix(
        aspectRatio,
        1.3f * 0.75f,
        0.1f,
        1000.0f
    );

    Camera* pCamera = createCamera(_camEntity, camProjMat);
    _camController.init();
    _camController.set(
        PLATY_MATH_PI * 0.25f, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 4, 0});

    Application::get_instance()->getInputManager().addWindowResizeEvent(
        new SceneWindowResizeEvent(this, _camEntity)
    );

    entityID_t dirLightEntity = createEntity();
    createDirectionalLight(dirLightEntity, { 0.5f, -0.5f, -0.5f }, { 1, 1, 1 });

    const float scaleModifier = 1;
    float areaScale = 60.0f * scaleModifier;

    // Load/generate all assets
    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    Image* pTerrainGrassImage = assetManager.loadImage("assets/textures/TerrainGrass.png");
    Image* pGrassImage = assetManager.loadImage("assets/textures/Grass.png");
    Image* pTreeTextureImage = assetManager.loadImage("assets/textures/FirTreeTexture.png");

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        1,
        0
    );
    Texture* pTerrainGrassTexture = assetManager.createTexture(
        pTerrainGrassImage->getID(),
        textureSampler
    );
    Texture* pGrassTexture = assetManager.createTexture(
        pGrassImage->getID(),
        textureSampler
    );
    Texture* pFirTreeTexture = assetManager.createTexture(
        pTreeTextureImage->getID(),
        textureSampler
    );
    Texture* pPineTreeTexture = assetManager.createTexture(
        pTreeTextureImage->getID(),
        textureSampler
    );

    Mesh* pGroundMesh = create_ground_mesh(assetManager, areaScale);
    Mesh* pGrassMesh = create_grass_mesh(assetManager, 1.25f);
    Model* pTreeModel1 = assetManager.loadModel("assets/models/FirTree.glb");
    Model* pTreeModel2 = assetManager.loadModel("assets/models/PineTree.glb");

    // Ground entity
    entityID_t groundEntity = createEntity();
    createTransform(
        groundEntity,
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1 }
    );
    createStaticMeshRenderable(
        groundEntity,
        pGroundMesh->getID(),
        pTerrainGrassTexture->getID()
    );

    // Grass entities
    size_t grassCount = 100 * scaleModifier * scaleModifier;
    createEntities(
        pGrassMesh->getID(),
        pGrassTexture->getID(),
        { 2, 2, 2 },
        areaScale,
        grassCount
    );

    // Tree entities
    size_t totalTreeCount = 50 * scaleModifier * scaleModifier;
    createEntities(
        pTreeModel1->getMeshes()[0]->getID(),
        pFirTreeTexture->getID(),
        { 0.5f, 0.5f, 0.5f },
        areaScale,
        totalTreeCount / 2
    );
    createEntities(
        pTreeModel2->getMeshes()[0]->getID(),
        pPineTreeTexture->getID(),
        { 0.5f, 0.5f, 0.5f },
        areaScale,
        totalTreeCount / 2
    );

    size_t totalRenderableCount = grassCount + totalTreeCount + 1;
    Debug::log("___TEST___Total renderable count: " + std::to_string(totalRenderableCount));
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

void TestScene::createEntities(
    ID_t meshID,
    ID_t textureID,
    const Vector3f transformScale,
    float areaScale,
    uint32_t count
)
{
    float halfAreaScale = areaScale * 0.5f;
    for (uint32_t i = 0; i < count; ++i)
    {
        entityID_t entity = createEntity();
        float randX = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);
        float randZ = (int)(-halfAreaScale) + (std::rand() % (int)areaScale);
        float randRot = (float)(std::rand() % 255);
        randRot = (randRot / 255.0f) * (float)(PLATY_MATH_PI * 2.0);
        createTransform(
            entity,
            { randX, 0, randZ },
            { {0, 1, 0}, randRot },
            transformScale
        );

        createStaticMeshRenderable(
            entity,
            meshID,
            textureID
        );
    }
}
