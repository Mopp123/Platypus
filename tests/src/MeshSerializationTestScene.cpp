#include "MeshSerializationTestScene.hpp"


using namespace platypus;


MeshSerializationTestScene::MeshSerializationTestScene()
{
}

MeshSerializationTestScene::~MeshSerializationTestScene()
{
}

void MeshSerializationTestScene::init()
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

    Light* pDirLight = (Light*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_LIGHT
    );
    pDirLight->direction = { 1.0f, -1.0f, 0.0f };
    pDirLight->direction = pDirLight->direction.normalize();

    const TextureSampler* pTextureSampler = pAssetManager->getOrCreateTextureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true
    );


    Material* pMaterial = pAssetManager->createMaterial(
        NULL_UUID,
        {
            pAssetManager->getErrorTexture()->getID()
        },
        {
            pAssetManager->getWhiteTexture()->getID()
        },
        { }
    );

    Model* pModel = pAssetManager->loadModel(
        "assets/TestCube.glb",
        false,
        "TestModel",
        NULL_UUID,
        { },
        true // store hostside buffers
    );
    Mesh* pMesh = pModel->getMeshes()[0];

    /*
    Debug::log("___TEST___Attempting to serialize mesh");
    std::vector<char> serializedMeshData;
    pMesh->writeToMetadataBuffer(serializedMeshData);

    Debug::log("___TEST___Attempting to write mesh");
    write_file("assets/serializationTest/TestCube.data", serializedMeshData);

    Debug::log("___TEST___Success!");
    PLATYPUS_ASSERT(false);
    */


    //std::vector<char> serializedData = read_file("assets/serializationTest/TestCube.data");
    //Debug::log("___TEST___Attempting to deserialize Mesh");
    //Mesh* pMesh = Mesh::create_from_metadata_buffer(pAssetManager, serializedData, 0);
    //Debug::log("___TEST___SUCCESS!");

    entityID_t entity = createEntity("TestEntity");
    create_transform(entity, Matrix4f(1.0f));
    create_renderable3D(entity, pMesh->getID(), pMaterial->getID());
}


void MeshSerializationTestScene::update()
{
    _camController.update();

    Application* pApp = Application::get_instance();
    InputManager& inputManager = pApp->getInputManager();
}
