#include "TestScene.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include "platypus/assets/Image.h"

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
    const Mesh* pMesh,
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
        pMesh->getID()
    );
    return testEntity;
}

void TestScene::init()
{
    float s = 1.0f;
    std::vector<float> vertexData = {
        -s, -s,     1, 0, 0,
        -s, s,      0, 1, 0,
        s, s,       1, 0, 1,
        s, -s,      1, 1, 0
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    Mesh* pMesh = Application::get_instance()->getAssetManager().createMesh(
        vertexData,
        indices
    );

    create_test_entity(
        this,
        pMesh,
        { 0.5f, 0, -1.5f },
        { { 0, 1, 0 }, 0 },
        { 0.5f, 0.5f, 0.5f }
    );

    testEntity = create_test_entity(
        this,
        pMesh,
        { 0.0f, 0, -2.0f },
        { { 0, 0, 1 }, 0.785f },
        { 0.5f, 0.5f, 0.5f }
    );


    // Test loading image
    AssetManager& assetManager = Application::get_instance()->getAssetManager();
    Image* pImage = assetManager.loadImage("assets/test.png");

    Debug::log("___TEST___initialized test scene!");
}

static float s_TEST_value = 0.0f;
void TestScene::update()
{
    s_TEST_value += 0.0001f;
    Matrix4f newMatrix = create_transformation_matrix(
        { 0.0f, 0, -2.0f },
        { { 0, 0, 1 }, s_TEST_value },
        { 0.4f, 0.4f, 0.4f }
    );
    Transform* pTransform = (Transform*)getComponent(testEntity, ComponentType::COMPONENT_TYPE_TRANSFORM);
    pTransform->globalMatrix = newMatrix;
}
