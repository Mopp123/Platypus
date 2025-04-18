#pragma once

#include "platypus/Platypus.h"

class TestScene : public platypus::Scene
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

    platypus::CameraController _camController;
    entityID_t _camEntity = NULL_ENTITY_ID;

public:
    TestScene();
    ~TestScene();
    virtual void init();
    virtual void update();

private:
    void createEntities(
        ID_t meshID,
        ID_t textureID,
        const platypus::Vector3f transformScale,
        float areaScale,
        uint32_t count
    );
};
