#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/ecs/Entity.h"

#include "platypus/core/InputEvent.h"


namespace platypus
{
    class CameraController
    {
    private:
        class ControllerCursorPosEvent : public CursorPosEvent
        {
        public:
            float& _pitchRef;
            float& _yawRef;
            int _lastX = 0;
            int _lastY = 0;
            float _rotationSpeed = 0.0f;
            ControllerCursorPosEvent(
                float& pitchRef,
                float& yawRef,
                float rotationSpeed
            );
            virtual void func(int x, int y);
        };

        class ControllerScrollEvent : public ScrollEvent
        {
        public:
            float& _zoomRef;
            float& _zoomSpeedRef;
            float& _maxZoomRef;
            ControllerScrollEvent(float& zoomRef, float& zoomSpeedRef, float& maxZoomRef) :
                _zoomRef(zoomRef),
                _zoomSpeedRef(zoomSpeedRef),
                _maxZoomRef(maxZoomRef)
            {}
            virtual void func(double xOffset, double yOffset);
        };

        entityID_t _cameraEntity = NULL_ENTITY_ID;
        Vector3f _offsetPosition = Vector3f(0, 0, 0);
        float _pitch = 0.0f;
        float _yaw = 0.0f;
        float _rotationSpeed = 0.0025f;
        float _zoom = 5.0f;
        float _maxZoom = 20.0f;
        float _zoomSpeed = 0.25f;

        float _movementSpeed = 5.0f;

        bool _initialized = false;

    public:
        void init();
        void update(Transform* pCameraTransform);

        void set(
            float pitch,
            float yaw,
            float rotationSpeed,
            float zoom,
            float maxZoom,
            float zoomSpeed
        );

        inline void setOffsetPos(const Vector3f& position) { _offsetPosition = position; }
    };
}
