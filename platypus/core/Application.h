#pragma once

#include "Window.h"
#include "InputManager.h"
#include "platypus/graphics/Context.h"
#include "platypus/graphics/Swapchain.h"

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Descriptors.h"

#include "platypus/graphics/renderers/MasterRenderer.h"


namespace platypus
{
    class Application
    {
    private:
        static Application* s_pInstance;

        Window _window;
        InputManager _inputManager;
        Context _context;

        // TESTING BELOW
        // NOTE: All rendering related stuff should probably contained somewhere else / in some "main renderer"
        // thing...
        Swapchain _swapchain;
        CommandPool _commandPool;
        DescriptorPool _descriptorPool;
        MasterRenderer _masterRenderer;

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

        void handleResize();

        static Application* get_instance();

        inline const Window& getWindow() const { return _window; }
        inline const InputManager& getInputManager() const { return _inputManager; }
        inline const Context& getContext() const { return _context; }
        inline const CommandPool& getCommandPool() const { return _commandPool; }
    };
}
