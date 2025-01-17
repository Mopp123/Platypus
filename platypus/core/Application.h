#pragma once

#include "Window.h"
#include "InputManager.h"
#include "platypus/graphics/Context.h"
#include "platypus/graphics/Swapchain.h"


namespace platypus
{
    class Application
    {
    private:
        static Application* s_pInstance;

        Window _window;
        InputManager _inputManager;
        Context _context;
        // NOTE: All rendering related stuff should probably contained somewhere else / in some "main renderer"
        // thing...
        Swapchain _swapchain;

    public:
        Application(
            const std::string& name,
            int width,
            int height,
            bool resizable,
            bool fullscreen
        );
        Application(const Application&) = delete;
        ~Application();

        void run();

        static Application* get_instance();

        inline const Window& getWindow() const { return _window; }
        inline const InputManager& getInputManager() const { return _inputManager; }
        inline Context& getContext() { return _context; }
    };
}
