#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class SkinnedMeshTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

    platypus::Pose _bindPose;
    std::vector<entityID_t> _jointEntities;

    size_t _selectedJointIndex = 0;
    entityID_t _selectedJointEntity = NULL_ENTITY_ID;

public:
    SkinnedMeshTestScene();
    ~SkinnedMeshTestScene();

    virtual void init();
    virtual void update();
};
