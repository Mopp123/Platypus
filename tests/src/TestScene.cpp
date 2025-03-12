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
    Debug::log("___TEST___created test scene!");
}

TestScene::~TestScene()
{
    Debug::log("___TEST___destroyed test scene!");
}

static entityID_t create_test_entity(
    Scene* pScene,
    ID_t meshID,
    ID_t textureID,
    const Vector3f& position,
    const Quaternion& rotation,
    const Vector3f& scale
)
{
    entityID_t testEntity = pScene->createEntity();
    pScene->createTransform(
        testEntity,
        position,
        rotation,
        scale
    );
    pScene->createStaticMeshRenderable(
        testEntity,
        meshID,
        textureID
    );
    return testEntity;
}

void TestScene::init()
{
    float s = 1.0f;
    std::vector<float> vertexData = {
        -s, -s,     0, 0,
        -s, s,      0, 1,
        s, s,       1, 1,
        s, -s,      1, 0
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    Mesh* pMesh = assetManager.createMesh(
        vertexData,
        indices
    );


    // Test loading image
    Image* pImage = assetManager.loadImage("assets/test.png");

    // Test creating sampler and texture
    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        1,
        0
    );
    Texture* pTexture = assetManager.createTexture(pImage->getID(), textureSampler);

    /*
    create_test_entity(
        this,
        pMesh->getID(),
        pTexture->getID(),
        { 0.5f, 0, -1.5f },
        { { 0, 1, 0 }, 0 },
        { 0.5f, 0.5f, 0.5f }
    );

    testEntity = create_test_entity(
        this,
        pMesh->getID(),
        pTexture->getID(),
        { 0.0f, 0, -2.0f },
        { { 0, 0, 1 }, 0.785f },
        { 0.5f, 0.5f, 0.5f }
    );
    */

    Model* pTestModel = assetManager.loadModel("assets/TestCube.glb");
    testEntity2 = createEntity();
    createTransform(
        testEntity2,
        { -0.5f, 0, -4 },
        { {0, 1, 0}, 0.78f },
        { 1, 1, 1 }
    );
    createStaticMeshRenderable(
        testEntity2,
        pTestModel->getMeshes()[0]->getID(),
        pTexture->getID()
    );

    Debug::log("___TEST___initialized test scene!");
}

static float s_TEST_value = 0.0f;
void TestScene::update()
{
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
}
