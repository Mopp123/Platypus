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

#include "SceneManager.h"
#include "platypus/assets/AssetManager.h"


namespace platypus
{
    class Application
    {
    private:
        static Application* s_pInstance;

        Window _window;
        InputManager _inputManager;
        SceneManager _sceneManager;
        Context _context;

        // TESTING BELOW
        // NOTE: All rendering related stuff should probably contained somewhere else / in some "main renderer"
        // thing...
        Swapchain _swapchain;
        CommandPool _commandPool;
        DescriptorPool _descriptorPool;
        MasterRenderer _masterRenderer;

        AssetManager _assetManager;

    public:
        Application(
            const std::string& name,
            int width,
            int height,
            bool resizable,
            bool fullscreen,
            Scene* pInitialScene
        );
        Application(const Application&) = delete;
        ~Application();

        void run();

        void handleResize();

        static Application* get_instance();

        inline const Window& getWindow() const { return _window; }
        inline InputManager& getInputManager() { return _inputManager; }
        inline SceneManager& getSceneManager() { return _sceneManager; }
        inline AssetManager& getAssetManager() { return _assetManager; }
        inline Context& getContext() { return _context; }
        inline const CommandPool& getCommandPool() const { return _commandPool; }
        inline MasterRenderer& getMasterRenderer() { return _masterRenderer; }
    };
}
