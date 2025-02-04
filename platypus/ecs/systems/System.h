#pragma once


namespace platypus
{
    class Scene;

    class System
    {
    public:
        System() = default;
        System(const System& other) = delete;
        virtual ~System() {}

        virtual void update(Scene* pScene) = 0;
    };
}
