#include "TestScene.h"
#include "UITestScene.h"
#include "platypus/ui/Button.h"


using namespace platypus;


class OnClickSwitchToUIScene : public ui::UIElement::OnClickEvent
{
public:
    OnClickSwitchToUIScene() {}
    virtual void func(MouseButtonName button, InputAction action)
    {
        if (action != InputAction::RELEASE)
        {
            Application::get_instance()->getSceneManager().assignNextScene(new UITestScene);
        }
    }
};

void TestScene::SceneWindowResizeEvent::func(int w, int h)
{
    Camera* pCameraComponent = (Camera*)_pScene->getComponent(
        _cameraEntity,
        ComponentType::COMPONENT_TYPE_CAMERA
    );

    float useHeight = h > 0 ? (float)h : 1.0f;
    float aspect = (float)w / useHeight;
    pCameraComponent->perspectiveProjectionMatrix = create_perspective_projection_matrix(
        aspect,
        _fov,
        _zNear,
        _zFar
    );

    pCameraComponent->orthographicProjectionMatrix = create_orthographic_projection_matrix(
        0,
        w,
        0,
        useHeight,
        0,
        0.1f
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
       -scale * 0.5f, 0, -scale * 0.5f,  0, 1, 0,  0, 0,  1, 0, 0, 1,
       -scale * 0.5f, 0,  scale * 0.5f,  0, 1, 0,  0, t,  1, 0, 0, 1,
        scale * 0.5f, 0,  scale * 0.5f,  0, 1, 0,  t, t,  1, 0, 0, 1,
        scale * 0.5f, 0, -scale * 0.5f,  0, 1, 0,  t, 0,  1, 0, 0, 1
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    VertexBufferLayout layout(
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float3 },
            { 2, ShaderDataType::Float2 },
            { 3, ShaderDataType::Float4 }
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    );
    return assetManager.createMesh(
        layout,
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

    VertexBufferLayout layout(
        {
            { 0, ShaderDataType::Float3 },
            { 1, ShaderDataType::Float3 },
            { 2, ShaderDataType::Float2 },
        },
        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
        0
    );
    return assetManager.createMesh(
        layout,
        vertexData,
        indices
    );
}

void TestScene::init()
{
    std::srand(23618);


    environmentProperties.clearColor = { 0.1f, 0.2f, 0.35f, 1.0f };

    _camEntity = createEntity();
    create_transform(
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

    Matrix4f perspectiveProjMat = create_perspective_projection_matrix(
        aspectRatio,
        1.3f * 0.75f,
        0.1f,
        1000.0f
    );

    Matrix4f orthoProjMat = create_orthographic_projection_matrix(
        0,
        windowSurfaceWidth,
        0,
        windowSurfaceHeight,
        0,
        0.1f
    );

    Camera* pCamera = create_camera(_camEntity, perspectiveProjMat, orthoProjMat);

    _camController.init();
    _camController.set(
        PLATY_MATH_PI * 0.25f, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _camController.setOffsetPos({ 0, 0, 0});

    Application::get_instance()->getInputManager().addWindowResizeEvent(
        new SceneWindowResizeEvent(this, _camEntity)
    );

    entityID_t dirLightEntity = createEntity();
    Vector3f lightDir(-1, -0.75f, -1);
    lightDir = lightDir.normalize();
    create_directional_light(dirLightEntity, lightDir, { 1, 1, 1 });

    const float scaleModifier = 1;
    float areaScale = 60.0f * scaleModifier;

    // Load/generate all assets
    AssetManager& assetManager = Application::get_instance()->getAssetManager();

    Image* pGrassImage = assetManager.loadImage("assets/textures/Grass.png");
    Image* pTreeTextureImage = assetManager.loadImage("assets/textures/FirTreeTexture.png");

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );
    Texture* pTerrainGrassTexture = assetManager.loadTexture(
        "assets/textures/TerrainGrass.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pTerrainGrassSpecularTexture = assetManager.loadTexture(
        "assets/textures/TerrainGrassSpecular.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pTerrainGrassNormalTexture = assetManager.loadTexture(
        "assets/textures/TerrainGrassNormal.png",
        ImageFormat::R8G8B8A8_UNORM,
        textureSampler
    );
    Texture* pGrassTexture = assetManager.createTexture(
        pGrassImage->getID(),
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Texture* pTreeTexture = assetManager.createTexture(
        pTreeTextureImage->getID(),
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    Material* pTerrainGrassMaterial = assetManager.createMaterial(
        pTerrainGrassTexture->getID(),
        pTerrainGrassSpecularTexture->getID(),
        pTerrainGrassNormalTexture->getID(),
        0.8f, // specular strength
        32.0f // shininess
    );
    Material* pGrassMaterial = assetManager.createMaterial(
        pGrassTexture->getID(),
        assetManager.getWhiteTexture()->getID(),
        NULL_ID,
        0.5f, // specular strength
        1.0f // shininess
    );
    Material* pTreeMaterial = assetManager.createMaterial(
        pTreeTexture->getID(),
        assetManager.getWhiteTexture()->getID(),
        NULL_ID,
        0.5f, // specular strength
        1.0f // shininess
    );

    Mesh* pGroundMesh = create_ground_mesh(assetManager, areaScale);
    Mesh* pGrassMesh = create_grass_mesh(assetManager, 1.25f);
    Model* pTreeModel1 = assetManager.loadModel("assets/models/FirTree.glb");
    Model* pTreeModel2 = assetManager.loadModel("assets/models/PineTree.glb");

    // Ground entity
    entityID_t groundEntity = createEntity();
    create_transform(
        groundEntity,
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0 },
        { 1, 1, 1 }
    );
    create_static_mesh_renderable(
        groundEntity,
        pGroundMesh->getID(),
        pTerrainGrassMaterial->getID()
    );

    // Grass entities
    size_t grassCount = 100 * scaleModifier * scaleModifier;
    createEntities(
        pGrassMesh->getID(),
        pGrassMaterial->getID(),
        { 2, 2, 2 },
        areaScale,
        grassCount
    );

    // Tree entities
    size_t totalTreeCount = 50 * scaleModifier * scaleModifier;
    createEntities(
        pTreeModel1->getMeshes()[0]->getID(),
        pTreeMaterial->getID(),
        { 0.5f, 0.5f, 0.5f },
        areaScale,
        totalTreeCount / 2
    );
    createEntities(
        pTreeModel2->getMeshes()[0]->getID(),
        pTreeMaterial->getID(),
        { 0.5f, 0.5f, 0.5f },
        areaScale,
        totalTreeCount / 2
    );

    size_t totalRenderableCount = grassCount + totalTreeCount + 1;
    Debug::log("___TEST___Total renderable count: " + std::to_string(totalRenderableCount));

    // Test combining 2d and 3d rendering using text box
    Font* pFont = assetManager.loadFont("assets/fonts/Ubuntu-R.ttf", 16);
    createTextBox(
        L"Testing text box",
        { 400, 300 },
        { 200, 50 },
        { 0.3f, 0.3f, 0.3f, 0.5f },
        1,
        pFont
    );

    createBox(
        { 400-10, 300-10 },
        { 200+20, 50+20 },
        { 0.25f, 0.25f, 0.25f, 0.5f },
        0
    );

    InputManager& inputManager = Application::get_instance()->getInputManager();
    _ui.init(this, inputManager);
    ui::Layout layout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.2f, 0.2f, 0.2f, 1.0f } // color
    };
    ui::UIElement* pContainer = ui::add_container(_ui, nullptr, layout, true);
    ui::UIElement* pButton = ui::add_button_element(
        _ui,
        pContainer,
        L"Switch Scene",
        pFont
    );
    pButton->_pOnClickEvent = new OnClickSwitchToUIScene;
}

static float s_testAnim = 0.0f;
void TestScene::update()
{
    Transform* pCamTransform = (Transform*)getComponent(_camEntity, ComponentType::COMPONENT_TYPE_TRANSFORM);

    s_testAnim += 0.5f * Timing::get_delta_time();

    _camController.set(
        PLATY_MATH_PI * 0.15f, // pitch
        s_testAnim,    // yaw
        0.0025f, // rot speed
        70.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );

    _camController.update(pCamTransform);
}

void TestScene::createEntities(
    ID_t meshID,
    ID_t materialID,
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
        create_transform(
            entity,
            { randX, 0, randZ },
            { {0, 1, 0}, randRot },
            transformScale
        );

        create_static_mesh_renderable(
            entity,
            meshID,
            materialID
        );
    }
}

void TestScene::createBox(
    const Vector2f& pos,
    const Vector2f& scale,
    const Vector4f& color,
    uint32_t layer
)
{
    entityID_t boxEntity = createEntity();
    create_gui_transform(
        boxEntity,
        pos,
        scale
    );
    GUIRenderable* pBoxRenderable = create_gui_renderable(
        boxEntity,
        color
    );
    pBoxRenderable->layer = layer;
}

void TestScene::createTextBox(
    const std::wstring& txt,
    const Vector2f& pos,
    const Vector2f& scale,
    const Vector4f& color,
    uint32_t layer,
    const Font* pFont
)
{
    createBox(pos, scale, color, layer);

    float padding = 5.0f;
    entityID_t textEntity = createEntity();
    create_gui_transform(
        textEntity,
        { pos.x + padding, pos.y + padding },
        { 1, 1 }
    );
    GUIRenderable* pTextRenderable = create_gui_renderable(
        textEntity,
        { 1, 1, 1, 1 }
    );
    pTextRenderable->textureID = pFont->getTextureID();
    pTextRenderable->fontID = pFont->getID();
    pTextRenderable->layer = layer + 1;

    pTextRenderable->text.resize(32);
    pTextRenderable->text = txt;
}
