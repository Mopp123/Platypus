#pragma once

#include "platypus/Platypus.h"
#include "BaseScene.h"
#include "platypus/assets/Material.h"
#include "platypus/ecs/Entity.h"


class HierarchyTestScene : public BaseScene
{
private:
    platypus::CameraController _camController;
    entityID_t _rootEntity = NULL_ENTITY_ID;
    std::vector<entityID_t> _entities;

    size_t _selectedIndex = 0;

public:
    HierarchyTestScene();
    ~HierarchyTestScene();

    virtual void init();
    virtual void update();
};
