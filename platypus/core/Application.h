#pragma once

#include "Window.h"
#include "InputManager.h"
#include "platypus/graphics/Context.h"
#include "platypus/graphics/renderers/MasterRenderer.h"
#include "SceneManager.h"
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
        Context _context;
        MasterRenderer _masterRenderer;
        SceneManager _sceneManager;
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

        static Application* get_instance();

        inline Window& getWindow() { return _window; }
        inline InputManager& getInputManager() { return _inputManager; }
        inline SceneManager& getSceneManager() { return _sceneManager; }
        inline AssetManager& getAssetManager() { return _assetManager; }
        inline Context& getContext() { return _context; }
        inline MasterRenderer& getMasterRenderer() { return _masterRenderer; }
    };
}
