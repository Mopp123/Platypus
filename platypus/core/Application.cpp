#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"
#include "Timing.h"

#include <chrono>

#ifdef PLATYPUS_BUILD_WEB
    #include "emscripten.h"
#endif

namespace platypus
{
    static int s_TEST_frames = 0;
    static std::chrono::time_point<std::chrono::high_resolution_clock> s_lastDisplayDelta;
    static void update()
    {
        Timing::update();

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        MasterRenderer* pRenderer = pApp->getMasterRenderer();

        pApp->getInputManager().pollEvents();
        sceneManager.update();
        if (sceneManager.getCurrentScene())
            pRenderer->render(pApp->getWindow());

        std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = currentTime - s_lastDisplayDelta;
        if (delta.count() >= 1.0f)
        {
            Debug::log("DELTA: " + std::to_string(Timing::get_delta_time()));
            s_lastDisplayDelta = std::chrono::high_resolution_clock::now();
        }

        /*
        if (s_TEST_frames >= 2)
        {
            float fps = 1.0f / Timing::get_delta_time();
            Debug::log("FPS: " + std::to_string(fps));
            s_TEST_frames = 0;
        }
        ++s_TEST_frames;
        */
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
        Context::create(name.c_str(), &_window);
        _pMasterRenderer = new MasterRenderer(_window);
        // NOTE: MasterRenderer shouldn't "own" commandPool since the commandPool is
        // used for non rendering related stuff as well!
        // TODO: Fix the above
        _pAssetManager = new AssetManager(_pMasterRenderer->getCommandPool());

        #ifdef PLATYPUS_DEBUG
            Debug::log(
                "Running platypus engine in DEBUG mode"
            );
        #endif

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

        Context::waitForOperations();
        // NOTE: Why the fuck was this commented out earlier!?!?!
        _pMasterRenderer->cleanUp();
        _inputManager.destroyEvents();
        _pAssetManager->destroyAssets();

        delete _pAssetManager;
        delete _pMasterRenderer;
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
