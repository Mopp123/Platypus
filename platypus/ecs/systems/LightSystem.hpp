#pragma once

#include "System.hpp"
#include "platypus/ecs/components/Lights.hpp"


namespace platypus
{
    class LightSystem : public System
    {
    public:
        LightSystem();
        ~LightSystem();

        void update(Scene* pScene);

    private:
        void updateDirectionalLight(Scene* pScene, Light* pDirectionalLight);
    };
}
