#include "Application.h"
#include "platypus/graphics/Device.hpp"
#include "Debug.h"
#include "Timing.h"

#include <chrono>

#ifdef PLATYPUS_BUILD_WEB
    #include "emscripten.h"
#endif

namespace platypus
{
    static std::chrono::time_point<std::chrono::high_resolution_clock> s_lastDisplayDelta;
    static void update()
    {
        Timing::update();

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        MasterRenderer* pRenderer = pApp->getMasterRenderer();

        pApp->getInputManager().pollEvents();

        std::chrono::time_point<std::chrono::high_resolution_clock> sceneBeginTime = std::chrono::high_resolution_clock::now();
        sceneManager.update();
        std::chrono::time_point<std::chrono::high_resolution_clock> sceneEndTime = std::chrono::high_resolution_clock::now();

        std::chrono::time_point<std::chrono::high_resolution_clock> renderBeginTime = std::chrono::high_resolution_clock::now();
        if (sceneManager.getCurrentScene())
            pRenderer->render(pApp->getWindow());
        std::chrono::time_point<std::chrono::high_resolution_clock> renderEndTime = std::chrono::high_resolution_clock::now();

        std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = currentTime - s_lastDisplayDelta;
        if (delta.count() >= 1.0f)
        {
            std::chrono::duration<float> sceneTime = sceneEndTime - sceneBeginTime;
            std::chrono::duration<float> renderTime = renderEndTime - renderBeginTime;
            Debug::log("DELTA: " + std::to_string(Timing::get_delta_time()) + " | Scene update took: " + std::to_string(sceneTime.count()) + " | Rendering took: " + std::to_string(renderTime.count()));
            s_lastDisplayDelta = std::chrono::high_resolution_clock::now();
        }

        sceneManager.handleSceneSwitching();
    }


    Application* Application::s_pInstance = nullptr;
    Application::Application(
        const std::string& name,
        int width,
        int height,
        bool resizable,
        WindowMode windowMode,
        Scene* pInitialScene
    ) :
        _window(name, width, height, resizable, windowMode),

        _inputManager(_window)
    {
        if (s_pInstance)
        {
            Debug::log(
                "@Application::Application "
                "Attempted to create Application but s_pInstance wasn't nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        s_pInstance = this;

        Context::create(name.c_str(), &_window);
        Device::create(&_window);

        // NOTE: Some fucking logic how core stuff is initialized and how their
        // lifetimes are controlled... This is fucking disgusting atm!
        //
        // *MasterRenderer creates only once "common shader resources" in its constructor
        // *MasterRenderer recreates all renderers' and Materials' shader resources on
        // window resize (on swapchain recreation)

        // NOTE: HUGE ISSUE:
        _pAssetManager = new AssetManager;
        _pSwapchain = new Swapchain(_window);
        _pMasterRenderer = new MasterRenderer(*_pSwapchain);

        #ifdef PLATYPUS_DEBUG
            Debug::log(
                "Running platypus engine in DEBUG mode"
            );
        #endif

        _sceneManager.assignNextScene(pInitialScene);
        s_lastDisplayDelta = std::chrono::high_resolution_clock::now();
    }

    Application::~Application()
    {
    }

    void Application::run()
    {
        _pMasterRenderer->createPipelines();
        #ifdef PLATYPUS_BUILD_DESKTOP
            while (!_window.isCloseRequested())
            {
                update();
            }
        #elif defined(PLATYPUS_BUILD_WEB)
            emscripten_set_main_loop(update, 0, 1);
        #endif

        Device::wait_for_operations();
        // NOTE: Need to destroy assets before destroying MasterRenderer because
        // some rely on descriptor pool that's living in the MasterRenderer atm!
        _pAssetManager->destroyAssets();
        // NOTE: Why the fuck was this commented out earlier!?!?!
        _pMasterRenderer->cleanUp();
        _inputManager.destroyEvents();

        delete _pMasterRenderer;
        // These shouldn't be accessed after this but ur so dumb,
        // that you'll forget -> this to at least see that these
        // are freed...
        _pMasterRenderer = nullptr;

        delete _pSwapchain;

        delete _pAssetManager;
        _pAssetManager = nullptr;


        Device::destroy();
        Context::destroy();
    }

    Application* Application::get_instance()
    {
        if (!s_pInstance)
        {
            Debug::log(
                "@Application::get_instance "
                "s_pInstance was nullptr! Make sure you have created Application explicitly.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return s_pInstance;
    }
}
