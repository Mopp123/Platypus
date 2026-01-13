#pragma once

#include "Window.hpp"
#include "InputManager.hpp"
#include "platypus/graphics/Context.hpp"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/renderers/MasterRenderer.h"
#include "SceneManager.hpp"
#include "platypus/assets/AssetManager.h"


namespace platypus
{
    class Application
    {
    private:
        static Application* s_pInstance;

        // Order of these is important for proper construction and destruction!
        Window _window;
        InputManager _inputManager;
        // TODO: Fix these -> not supposed to be heap allocated?
        Swapchain* _pSwapchain = nullptr;
        AssetManager* _pAssetManager = nullptr;
        MasterRenderer* _pMasterRenderer = nullptr;

        SceneManager _sceneManager;

    public:
        Application(
            const std::string& name,
            int width,
            int height,
            bool resizable,
            WindowMode windowMode,
            Scene* pInitialScene
        );
        Application(const Application&) = delete;
        ~Application();

        void run();

        static Application* get_instance();

        inline Window& getWindow() { return _window; }
        inline InputManager& getInputManager() { return _inputManager; }
        inline SceneManager& getSceneManager() { return _sceneManager; }
        inline Swapchain* getSwapchain() { return _pSwapchain; }
        inline AssetManager* getAssetManager() { return _pAssetManager; }
        inline MasterRenderer* getMasterRenderer() { return _pMasterRenderer; }
    };
}
