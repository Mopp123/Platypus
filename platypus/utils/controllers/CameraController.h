#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/ecs/Entity.h"


namespace platypus
{
    class CameraController
    {
    private:
        entityID_t _cameraEntity = NULL_ENTITY_ID;
        Vector3f _offsetPoint;
        float _yaw = 0.0f;
        float _pitch = 0.0f;
        float _zoom = 5.0f;
        float _speed = 2.0f;

    public:
        void init(entityID_t cameraEntity);
        void update();
    };
}
