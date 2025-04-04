#include "CameraController.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    void CameraController::init(entityID_t cameraEntity)
    {
        _cameraEntity = cameraEntity;
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene)
        {
            Debug::log(
                "@CameraController::init "
                "No current scene found!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        Transform* pTransform = pScene.getComponent(_cameraEntity);
        if (!pTransform)
        {
            Debug::log(
                "@CameraController::init "
                "No transform found for entity: " + std::to_string(_cameraEntity) + " "
                "Transform component is required to use CameraController!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        Camera* pCamera = pScene.getComponent(_cameraEntity);
        if (!pCamera)
        {
            Debug::log(
                "@CameraController::init "
                "No camera found for entity: " + std::to_string(_cameraEntity) + " "
                "Camera component is required to use CameraController!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void CameraController::update()
    {
        float& xPos = _pivotPoint.x;
        float& zPos = _pivotPoint.z;

        float rotatedAngle = _yaw + M_PI * 0.5f;

        if (_moveForward)
        {
            xPos += std::cos(rotatedAngle) * _speed * Timing::get_delta_time();
            zPos += -std::sin(rotatedAngle) * _speed * Timing::get_delta_time();
        }
        else if (_moveBackwards)
        {
            xPos += std::cos(rotatedAngle) * -_speed * Timing::get_delta_time();
            zPos += -std::sin(rotatedAngle) * -_speed * Timing::get_delta_time();
        }

        if (_moveLeft)
        {
            xPos += std::cos(_yaw) * -_speed * Timing::get_delta_time();
            zPos += -std::sin(_yaw) * -_speed * Timing::get_delta_time();
        }
        else if (_moveRight)
        {
            xPos += std::cos(_yaw) * _speed * Timing::get_delta_time();
            zPos += -std::sin(_yaw) * _speed * Timing::get_delta_time();
        }

        // calc cam transform according to pivot point and angles
        float horizontalDist = std::cos(_pitch) * _distToPivotPoint;
        float verticalDist = std::sin(_pitch) * _distToPivotPoint;

        mat4 translationMat;
        translationMat.setIdentity();
        translationMat[0 + 3 * 4] = _pivotPoint.x + std::sin(_yaw) * horizontalDist;
        translationMat[1 + 3 * 4] = _pivotPoint.y + verticalDist;
        translationMat[2 + 3 * 4] = _pivotPoint.z + std::cos(_yaw) * horizontalDist;

        mat4 rotMatYaw;
        rotMatYaw.setIdentity();
        rotMatYaw[0 + 0 * 4] = std::cos(_yaw);
        rotMatYaw[0 + 2 * 4] = std::sin(_yaw);
        rotMatYaw[2 + 0 * 4] = -std::sin(_yaw);
        rotMatYaw[2 + 2 * 4] = std::cos(_yaw);

        mat4 rotMatPitch;
        rotMatPitch.setIdentity();
        rotMatPitch[1 + 1 * 4] = std::cos(-_pitch);
        rotMatPitch[1 + 2 * 4] = -std::sin(-_pitch);
        rotMatPitch[2 + 1 * 4] = std::sin(-_pitch);
        rotMatPitch[2 + 2 * 4] = std::cos(-_pitch);

        Transform* pTransform = pScene.getComponent(_cameraEntity);
        pTransform->globalMatrix = translationMat * rotMatYaw * rotMatPitch;
    }
}
