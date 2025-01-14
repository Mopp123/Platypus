#pragma once

#include "Window.h"
#include "InputManager.h"


namespace platypus
{
    class Application
    {
    private:
        static Application* s_pInstance;

        Window* _pWindow = nullptr;
        InputManager* _pInputManager = nullptr;

    public:
        Application(Window* pWindow, InputManager* pInputManager);
        Application(const Application&) = delete;
        ~Application();

        void run();

        static Application* get_instance();

        inline const Window* getWindow() const { return _pWindow; }
        inline const InputManager* getInputManager() const { return _pInputManager; }
    };
}
