#include "MaterialTestScene.h"
#include "UITestScene.h"

using namespace platypus;


MaterialTestScene::MaterialTestScene()
{
}

MaterialTestScene::~MaterialTestScene()
{
}

void MaterialTestScene::init()
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
        L"Material Test",
        { 1, 1, 1, 1 },
        pFont
    );

    _camController.init(_cameraEntity);
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
        true,
        0
    );

    Texture* pDiffuseTexture = assetManager.loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pSpecularTexture = assetManager.loadTexture(
        "assets/textures/SpecularTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pNormalTexture = assetManager.loadTexture(
        "assets/textures/NormalTest.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );
    Texture* pFloorTexture = assetManager.loadTexture(
        "assets/textures/Floor.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pFloorSpecularTexture = assetManager.loadTexture(
        "assets/textures/FloorSpecular.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pFloorNormalTexture = assetManager.loadTexture(
        "assets/textures/FloorNormal.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );

    Material* pMaterial = assetManager.createMaterial(
        pDiffuseTexture->getID(),
        assetManager.getWhiteTexture()->getID(),
        pNormalTexture->getID(),
        0.8f,
        16.0f
    );
    Material* pMaterial2 = assetManager.createMaterial(
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

    Model* pModel = assetManager.loadModel("assets/TestCubeTangents.glb");
    Model* pModel2 = assetManager.loadModel("assets/TestCube.glb");
    Model* pFloorModel = assetManager.loadModel("assets/models/Floor.glb");

    // Create box entities
    // Normal mapped
    _boxEntity = createEntity();
    Transform* pTransform = create_transform(
        _boxEntity,
        pModel->getMeshes()[0]->getTransformationMatrix()
    );
    pTransform->globalMatrix[2 + 3 * 4] = -12.0f;
    StaticMeshRenderable* pRenderable = create_static_mesh_renderable(
        _boxEntity,
        pModel2->getMeshes()[0]->getID(),
        pMaterial2->getID()
    );
    // Non normal mapped
    entityID_t entity2 = createEntity();
    Transform* pTransform2 = create_transform(
        entity2,
        pModel2->getMeshes()[0]->getTransformationMatrix()
    );
    pTransform2->globalMatrix[2 + 3 * 4] = -12.0f;
    pTransform2->globalMatrix[0 + 3 * 4] = -6.0f;
    StaticMeshRenderable* pRenderable2 = create_static_mesh_renderable(
        entity2,
        pModel2->getMeshes()[0]->getID(),
        pMaterial2->getID()
    );

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

static float s_anim = 0.0f;
void MaterialTestScene::update()
{
    updateBase();

    // Rotate box
    Transform* pBoxTransform = (Transform*)getComponent(
        _boxEntity,
        ComponentType::COMPONENT_TYPE_TRANSFORM
    );
    Vector3f rotationAngle(0, 1, 1);
    rotationAngle = rotationAngle.normalize();
    Matrix4f boxTransformationMatrix = create_transformation_matrix(
        { -2, 3, -12 },
        { rotationAngle, s_anim },
        { 1, 1, 1 }
    );
    s_anim += 2.0f * Timing::get_delta_time();
    pBoxTransform->globalMatrix = boxTransformationMatrix;

    _camController.update();

    InputManager& inputManager = Application::get_instance()->getInputManager();
    if(inputManager.isKeyDown(KeyName::KEY_0))
    {
        Application::get_instance()->getSceneManager().assignNextScene(new UITestScene);
    }
}
