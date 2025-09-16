#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"


class SkinnedMeshTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;

    std::vector<entityID_t> _rootJointEntities;

    platypus::SkeletalAnimationData* _pRunAnimationAsset = nullptr;
    platypus::SkeletalAnimationData* _pIdleAnimationAsset = nullptr;

    size_t _selectedJointIndex = 0;
    entityID_t _selectedJointEntity = NULL_ENTITY_ID;

public:
    SkinnedMeshTestScene();
    ~SkinnedMeshTestScene();

    virtual void init();
    virtual void update();
};
