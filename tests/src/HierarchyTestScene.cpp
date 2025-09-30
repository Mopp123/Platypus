#include "HierarchyTestScene.hpp"
#include "platypus/utils/Maths.h"

using namespace platypus;

HierarchyTestScene::HierarchyTestScene()
{
}

HierarchyTestScene::~HierarchyTestScene()
{
}

void HierarchyTestScene::init()
{
    initBase();

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

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
        true,
        0
    );
    Texture* pDiffuseTexture = pAssetManager->loadTexture(
        "assets/textures/DiffuseTest.png",
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );

    Material* pMaterial = pAssetManager->createMaterial(
        MaterialType::MESH,
        NULL_ID,
        { pDiffuseTexture->getID() },
        { pAssetManager->getWhiteTexture()->getID() },
        { },
        0.8f,
        16.0f
    );

    Model* pModel = pAssetManager->loadModel("assets/TestCube.glb");

    _rootEntity = createEntity();
    create_transform(
        _rootEntity,
        { 0, 0, 0 },
        { { 0, 1, 0 }, 0.0f },
        { 1, 1, 1}
    );
    create_static_mesh_renderable(
        _rootEntity,
        pModel->getMeshes()[0]->getID(),
        pMaterial->getID()
    );

    entityID_t childEntity = createEntity();
    create_transform(
        childEntity,
        { 0, 2, 0 },
        { { 0, 1, 0 }, 0.0f },
        { 1, 1, 1}
    );
    create_static_mesh_renderable(
        childEntity,
        pModel->getMeshes()[0]->getID(),
        pMaterial->getID()
    );
    add_child(_rootEntity, childEntity);

    entityID_t childEntity2 = createEntity();
    create_transform(
        childEntity2,
        { 0, 2, 0 },
        { { 0, 1, 0 }, 0.0f },
        { 1, 1, 1}
    );
    create_static_mesh_renderable(
        childEntity2,
        pModel->getMeshes()[0]->getID(),
        pMaterial->getID()
    );
    add_child(childEntity, childEntity2);

    _entities.push_back(_rootEntity);
    _entities.push_back(childEntity);
    _entities.push_back(childEntity2);
}

static bool s_keyDown = false;
void HierarchyTestScene::update()
{
    _camController.update();

    // test moving entities in relation to their parents
    InputManager& inputManager = Application::get_instance()->getInputManager();
    if (inputManager.isKeyDown(KeyName::KEY_TAB) && !s_keyDown)
    {
        s_keyDown = true;
        _selectedIndex = (_selectedIndex + 1) % _entities.size();
    }
    if (!inputManager.isKeyDown(KeyName::KEY_TAB) && s_keyDown)
        s_keyDown = false;

    entityID_t selectedEntityID = _entities[_selectedIndex];
    Transform* pSelectedTransform = (Transform*)getComponent(
        selectedEntityID,
        ComponentType::COMPONENT_TYPE_TRANSFORM
    );

    const float rotSpeed = 5.0f;
    if (inputManager.isKeyDown(KeyName::KEY_LEFT))
    {
        rotate_transform(
            pSelectedTransform,
            0,
            -rotSpeed * Timing::get_delta_time(),
            0,
            selectedEntityID != _rootEntity
        );
    }
    else if (inputManager.isKeyDown(KeyName::KEY_RIGHT))
    {
        rotate_transform(
            pSelectedTransform,
            0,
            rotSpeed * Timing::get_delta_time(),
            0,
            selectedEntityID != _rootEntity
        );
    }

    if (inputManager.isKeyDown(KeyName::KEY_UP))
    {
        rotate_transform(
            pSelectedTransform,
            rotSpeed * Timing::get_delta_time(),
            0,
            0,
            selectedEntityID != _rootEntity
        );
    }
    else if (inputManager.isKeyDown(KeyName::KEY_DOWN))
    {
        rotate_transform(
            pSelectedTransform,
            -rotSpeed * Timing::get_delta_time(),
            0,
            0,
            selectedEntityID != _rootEntity
        );
    }

}
