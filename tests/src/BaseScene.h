#pragma once

#include "platypus/Platypus.h"


class BaseScene : public platypus::Scene
{
private:
    class SceneWindowResizeEvent : public platypus::WindowResizeEvent
    {
    public:
        platypus::Scene* _pScene = nullptr;
        entityID_t _cameraEntity;
        float _fov = 1.3f * 0.75f;
        float _zNear = 0.1f;
        float _zFar = 1000.0f;
        SceneWindowResizeEvent(platypus::Scene* pScene, entityID_t cameraEntity) :
            _pScene(pScene),
            _cameraEntity(cameraEntity)
        {}
        virtual void func(int w, int h);
    };

protected:
    platypus::CameraController _cameraController;
    entityID_t _cameraEntity = NULL_ENTITY_ID;
    entityID_t _lightEntity = NULL_ENTITY_ID;

public:
    BaseScene();
    virtual ~BaseScene();
    void initBase();
    void updateBase();
};
