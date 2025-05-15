#include "MeshTestScene.h"
#include "UITestScene.h"

using namespace platypus;


MeshTestScene::MeshTestScene()
{
}

MeshTestScene::~MeshTestScene()
{
}

void MeshTestScene::init()
{
    initBase();

    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    InputManager& inputManager = Application::get_instance()->getInputManager();
    _ui.init(this, inputManager);
    ui::Layout layout {
        { 0, 0 }, // pos
        { 120, 40 }, // scale
        { 0.2f, 0.2f, 0.2f, 1.0f } // color
    };
    layout.padding = { 10, 10 };
    layout.elementGap = 10;
    layout.horizontalContentAlignment = ui::HorizontalAlignment::LEFT;
    layout.verticalContentAlignment = ui::VerticalAlignment::TOP;

    ui::UIElement* pUIContainer = add_container(
        _ui,
        nullptr,
        layout,
        true
    );

    Font* pFont = assetManager.loadFont("assets/fonts/Ubuntu-R.ttf", 16);
    ui::UIElement* pText =  ui::add_text_element(
        _ui,
        pUIContainer,
        L"Mesh testing",
        { 1, 1, 1, 1 },
        pFont
    );

    _camController.init();
    _camController.set(
        0, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 0, 0});


    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        1,
        0
    );

    Texture* pDiffuseTexture = assetManager.loadTexture(
        "assets/textures/DiffuseTest.png",
        textureSampler
    );
    Texture* pSpecularTexture = assetManager.loadTexture(
        "assets/textures/SpecularTest.png",
        textureSampler
    );
    Texture* pFloorTexture = assetManager.loadTexture(
        "assets/textures/Floor.png",
        textureSampler
    );
    Texture* pFloorSpecularTexture = assetManager.loadTexture(
        "assets/textures/FloorSpecular.png",
        textureSampler
    );
    Texture* pFloorNormalTexture = assetManager.loadTexture(
        "assets/textures/FloorNormal.png",
        textureSampler
    );

    Material* pMaterial = assetManager.createMaterial(
        pDiffuseTexture->getID(),
        assetManager.getWhiteTexture()->getID(),
        NULL_ID,
        0.8f,
        16.0f
    );
    Material* pFloorMaterial = assetManager.createMaterial(
        pFloorTexture->getID(),
        pFloorSpecularTexture->getID(),
        pFloorNormalTexture->getID(),
        0.8f,
        64.0f
    );

    Model* pModel = assetManager.loadModel("assets/TestCube.glb");
    Model* pFloorModel = assetManager.loadModel("assets/models/Floor.glb");

    // Create box entities
    for (Mesh* pMesh : pModel->getMeshes())
    {
        entityID_t entity = createEntity();
        Transform* pTransform = create_transform(
            entity,
            pMesh->getTransformationMatrix()
        );

        pTransform->globalMatrix[2 + 3 * 4] = -12.0f;

        StaticMeshRenderable* pRenderable = create_static_mesh_renderable(
            entity,
            pMesh->getID(),
            pMaterial->getID()
        );
        break;
    }

    // Create floor entity
    entityID_t floorEntity = createEntity();
    Transform* pFloorTransform = create_transform(
        floorEntity,
        { 0, 0, -12 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1 }
    );
    StaticMeshRenderable* pFloorRenderable = create_static_mesh_renderable(
        floorEntity,
        pFloorModel->getMeshes()[0]->getID(),
        pFloorMaterial->getID()
    );


    DirectionalLight* pDirLight = (DirectionalLight*)getComponent(
        _lightEntity,
        ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
    );
    pDirLight->direction = { 0.75f, -1.5f, -1.0f };
}


void MeshTestScene::update()
{
    updateBase();

    Transform* pCamTransform = (Transform*)getComponent(
        _cameraEntity,
        ComponentType::COMPONENT_TYPE_TRANSFORM
    );
    _camController.update(pCamTransform);

    InputManager& inputManager = Application::get_instance()->getInputManager();
    if(inputManager.isKeyDown(KeyName::KEY_0))
    {
        Application::get_instance()->getSceneManager().assignNextScene(new UITestScene);
    }
}
