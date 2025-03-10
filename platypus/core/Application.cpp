#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"
#include "Timing.h"

#ifdef PLATYPUS_BUILD_WEB
    #include "emscripten.h"
#endif

namespace platypus
{
    static int s_TEST_frames = 0;
    static void update()
    {
        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        MasterRenderer& renderer = pApp->getMasterRenderer();

        pApp->getInputManager().pollEvents();
        sceneManager.update();
        renderer.render(pApp->getWindow());

        Timing::update();
        if (s_TEST_frames >= 1000)
        {
            float fps = 1.0f / Timing::get_delta_time();
            Debug::log("FPS: " + std::to_string(fps));
            s_TEST_frames = 0;
        }
        ++s_TEST_frames;

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
        _inputManager(_window),
        _context(name.c_str(), &_window),
        _masterRenderer(_window),
        // NOTE: MasterRenderer shouldn't "own" commandPool since the commandPool is
        // used for non rendering related stuff as well!
        // TODO: Fix the above
        _assetManager(_masterRenderer.getCommandPool())
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

        _sceneManager.assignNextScene(pInitialScene);
    }

    Application::~Application()
    {
    }

    void Application::run()
    {
        #ifdef PLATYPUS_BUILD_DESKTOP
            while (!_window.isCloseRequested())
            {
                update();
            }
        #elif defined(PLATYPUS_BUILD_WEB)
            emscripten_set_main_loop(update, 0, 1);
        #endif

        _context.waitForOperations();
        //_masterRenderer.cleanUp();
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
