#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"
#include "Timing.h"


namespace platypus
{
    Application* Application::s_pInstance = nullptr;

    Application::Application(
        const std::string& name,
        int width,
        int height,
        bool resizable,
        bool fullscreen,
        Scene* pInitialScene
    ) :
        _window(name, width, height, resizable, fullscreen),
        _inputManager(_window),
        _context(name.c_str(), &_window),
        _masterRenderer(_window),
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

    static int s_TEST_frames = 0;
    void Application::run()
    {
        while (!_window.isCloseRequested())
        {
            _inputManager.pollEvents();

            _sceneManager.update();

            _masterRenderer.render(_window);

            Timing::update();
            if (s_TEST_frames >= 1000)
            {
                float fps = 1.0f / Timing::get_delta_time();
                Debug::log("FPS: " + std::to_string(fps));
                s_TEST_frames = 0;
            }
            ++s_TEST_frames;

            _sceneManager.handleSceneSwitching();
        }
        _context.waitForOperations();

        _masterRenderer.cleanUp();
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
