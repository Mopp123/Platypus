#include "CameraController.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/core/Application.h"
#include "platypus/core/Timing.h"
#include "platypus/core/Debug.h"
#include <cmath>


namespace platypus
{
    CameraController::ControllerCursorPosEvent::ControllerCursorPosEvent(
        float& pitchRef,
        float& yawRef,
        float rotationSpeed
    ) :
        _pitchRef(pitchRef),
        _yawRef(yawRef),
        _rotationSpeed(rotationSpeed)
    {
        Application* pApp = Application::get_instance();
        InputManager& inputManager = pApp->getInputManager();
        _lastX = inputManager.getMouseX();
        _lastY = inputManager.getMouseY();
    }

    void CameraController::ControllerCursorPosEvent::func(int x, int y)
    {
        float dx = x - _lastX;
        float dy = y - _lastY;

        _lastX = x;
        _lastY = y;

        Application* pApp = Application::get_instance();
        InputManager& inputManager = pApp->getInputManager();
        if (inputManager.isMouseButtonDown(MouseButtonName::MOUSE_LEFT))
        {
            _yawRef += dx * _rotationSpeed;
            float newPitch = _pitchRef + dy * _rotationSpeed;
            if (newPitch >= 0.0f && newPitch <= PLATY_MATH_PI * 0.5f)
                _pitchRef = newPitch;
        }
    }


    void CameraController::ControllerScrollEvent::func(double xOffset, double yOffset)
    {
        float val = yOffset > 0.0 ? 1.0f : -1.0f;
        float newZoom = _zoomRef - val * _zoomSpeedRef;
        if (newZoom >= 0.0f && newZoom <= _maxZoomRef)
            _zoomRef = newZoom;
    }


    void CameraController::init(entityID_t cameraEntity)
    {
        _cameraEntity = cameraEntity;
        Application* pApp = Application::get_instance();
        InputManager& inputManager = pApp->getInputManager();

        inputManager.addCursorPosEvent(new ControllerCursorPosEvent(_pitch, _yaw, _rotationSpeed));
        inputManager.addScrollEvent(new ControllerScrollEvent(_zoom, _zoomSpeed, _maxZoom));
        _initialized = true;
    }

    void CameraController::update()
    {
        if (!_initialized)
        {
            Debug::log(
                "@CameraController::update "
                "CameraController was not initialized! "
                "Make sure you have initialized with init() before updating",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // NOTE: This is done in a way that _yaw=0 is pointing towards -z axis
        float halfPI = PLATY_MATH_PI * 0.5f;
        float useYaw = _yaw + halfPI;

        Application* pApp = Application::get_instance();
        InputManager& inputManager = pApp->getInputManager();

        float moveAmount = _movementSpeed * Timing::get_delta_time();
        if (inputManager.isKeyDown(KeyName::KEY_W))
        {
            _offsetPosition.x -= std::cos(useYaw) * moveAmount;
            _offsetPosition.z -= std::sin(useYaw) * moveAmount;
        }
        else if (inputManager.isKeyDown(KeyName::KEY_S))
        {
            _offsetPosition.x += std::cos(useYaw) * moveAmount;
            _offsetPosition.z += std::sin(useYaw) * moveAmount;
        }

        if (inputManager.isKeyDown(KeyName::KEY_D))
        {
            _offsetPosition.x -= std::cos(useYaw + halfPI) * moveAmount;
            _offsetPosition.z -= std::sin(useYaw + halfPI) * moveAmount;
        }
        else if (inputManager.isKeyDown(KeyName::KEY_A))
        {
            _offsetPosition.x += std::cos(useYaw + halfPI) * moveAmount;
            _offsetPosition.z += std::sin(useYaw + halfPI) * moveAmount;
        }

        float horizontalDist = cos(_pitch) * _zoom;
        float verticalDist = sin(_pitch) * _zoom;

        Matrix4f translationMatrix(1.0f);
        translationMatrix[0 + 3 * 4] = _offsetPosition.x + cos(useYaw) * horizontalDist;
        translationMatrix[1 + 3 * 4] = _offsetPosition.y + verticalDist ;
        translationMatrix[2 + 3 * 4] = _offsetPosition.z + sin(useYaw) * horizontalDist;

        Transform* pTransform = (Transform*)pApp->getSceneManager().getCurrentScene()->getComponent(
            _cameraEntity,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        pTransform->globalMatrix = translationMatrix * create_rotation_matrix(-_pitch, -_yaw, 0.0f);
    }

    void CameraController::set(
        float pitch,
        float yaw,
        float rotationSpeed,
        float zoom,
        float maxZoom,
        float zoomSpeed
    )
    {
        _pitch = pitch;
        _yaw = yaw;
        _rotationSpeed = rotationSpeed;
        _zoom = zoom;
        _maxZoom = maxZoom;
        _zoomSpeed = zoomSpeed;
    }
}
