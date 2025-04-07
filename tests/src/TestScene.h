#pragma once

#include "platypus/core/Scene.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/utils/controllers/CameraController.h"
#include "platypus/core/InputEvent.h"

class TestScene : public platypus::Scene
{
private:
    class SceneWindowResizeEvent : public platypus::WindowResizeEvent
    {
    public:
        platypus::Scene* _pScene = nullptr;
        entityID_t _cameraEntity;
        float _fov = 1.3f;
        float _zNear = 0.1f;
        float _zFar = 100.0f;
        SceneWindowResizeEvent(platypus::Scene* pScene, entityID_t cameraEntity) :
            _pScene(pScene),
            _cameraEntity(cameraEntity)
        {}
        virtual void func(int w, int h);
    };

    platypus::CameraController _camController;
    entityID_t _camEntity = NULL_ENTITY_ID;

public:
    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();
};
