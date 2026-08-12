#include "SerializationTestScene.hpp"


using namespace platypus;


SerializationTestScene::SerializationTestScene()
{
}

SerializationTestScene::~SerializationTestScene()
{
}

void SerializationTestScene::init()
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
        "assets/models/MultiAnimSkeletonTest.glb",
        false,
        "TestModel",
        NULL_UUID,
        { },
        true // store hostside buffers
    );
    Mesh* pMesh = pModel->getMeshes()[0];

    Debug::log("___TEST___Attempting to serialize mesh");
    std::vector<char> serializedData;
    pMesh->serialize(serializedData);

    for (size_t i = 0; i < pMesh->getAnimations().size(); ++i)
    {
        const SkeletalAnimationData* pAnimData = pMesh->getAnimations()[i];
        Debug::log("___TEST___Attempting to serialize animation[" + std::to_string(i) + "] name: " + pAnimData->getName() + " length = " + std::to_string(pAnimData->getLength()));
        pAnimData->serialize(serializedData);
    }

    Debug::log("___TEST___Attempting to write");
    write_file("assets/serializationTest/Test.data", serializedData);

    /*
    std::vector<char> serializedData = read_file("assets/serializationTest/Test.data");
    Debug::log("___TEST___Attempting to deserialize mesh");
    Mesh* pMesh = new Mesh(pAssetManager, serializedData, 0);
    size_t pos = pMesh->getSerializedSize();

    Debug::log("___TEST___Attempting to deserialize anim1");
    SkeletalAnimationData* pAnimData1 = new SkeletalAnimationData(pAssetManager, serializedData, pos);
    pos += pAnimData1->getSerializedSize();

    Debug::log("___TEST___Attempting to deserialize anim2");
    SkeletalAnimationData* pAnimData2 = new SkeletalAnimationData(pAssetManager, serializedData, pos);
    pos += pAnimData2->getSerializedSize();

    Debug::log("___TEST___Deserialized anims:");

    Debug::log("    " + pAnimData1->getName());
    Debug::log("        length: " + std::to_string(pAnimData1->getLength()));

    Debug::log("    " + pAnimData2->getName());
    Debug::log("        length: " + std::to_string(pAnimData2->getLength()));
    */

    Debug::log("___TEST___SUCCESS!");

}


void SerializationTestScene::update()
{
    _camController.update();

    Application* pApp = Application::get_instance();
    InputManager& inputManager = pApp->getInputManager();
}
