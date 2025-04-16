#include "BaseScene.h"

using namespace platypus;


void BaseScene::SceneWindowResizeEvent::func(int w, int h)
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


BaseScene::BaseScene()
{}

BaseScene::~BaseScene()
{}

void BaseScene::initBase()
{
    environmentProperties.clearColor = { 0.5f, 0.5f, 0.5f, 1.0f };

    _cameraEntity = createEntity();
    createTransform(
        _cameraEntity,
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

    Debug::log("___TEST___window scale: " + std::to_string(windowSurfaceWidth) + ", " + std::to_string(windowSurfaceHeight));

    Matrix4f orthoProjMat = create_orthographic_projection_matrix(
        -1,
        1, //windowSurfaceWidth,
        1,
        -1,//windowSurfaceHeight,
        0,
        1
    );

    Camera* pCamera = createCamera(_cameraEntity, perspectiveProjMat, orthoProjMat);
    /*
    _cameraController.init();
    _cameraController.set(
        PLATY_MATH_PI * 0.25f, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _cameraController.setOffsetPos({ 0, 4, 0});
    */

    Application::get_instance()->getInputManager().addWindowResizeEvent(
        new SceneWindowResizeEvent(this, _cameraEntity)
    );
}

void BaseScene::updateBase()
{}
