#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"

#include "platypus/graphics/Buffers.h"


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
        _swapchain(_window),
        _descriptorPool(_swapchain),
        // NOTE: Not sure is CommanPool created at this point -> fucks up master renderer creation if not!
        _masterRenderer(_swapchain, _commandPool, _descriptorPool)
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

        _masterRenderer.allocCommandBuffers(_swapchain.getMaxFramesInFlight());
        _masterRenderer.createPipelines(_swapchain);

        _sceneManager.assignNextScene(pInitialScene);
    }

    Application::~Application()
    {
    }

    void Application::run()
    {
        while (!_window.isCloseRequested())
        {
            _inputManager.pollEvents();

            _sceneManager.update();

            // NOTE: Below should probably be done somewhere else...?
            SwapchainResult result = _swapchain.acquireImage();
            if (result == SwapchainResult::ERROR)
            {
                Debug::log(
                    "@Application::run "
                    "Failed to acquire swapchain image!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            else if (result == SwapchainResult::RESIZE_REQUIRED)
            {
                handleResize();
            }
            else
            {
                const CommandBuffer& cmdBuf = _masterRenderer.recordCommandBuffer(_swapchain, _swapchain.getCurrentFrame());
                _context.submitPrimaryCommandBuffer(_swapchain, cmdBuf, _swapchain.getCurrentFrame());

                // present may also tell us to recreate swapchain!
                if (_swapchain.present() == SwapchainResult::RESIZE_REQUIRED || _window.resized())
                    handleResize();
            }

            _sceneManager.handleSceneSwitching();
        }
        _context.waitForOperations();

        _masterRenderer.cleanUp();
    }

    void Application::handleResize()
    {
        _context.waitForOperations();
        if (!_window.isMinimized())
        {
            _context.handleWindowResize();
            _swapchain.recreate(_window);
            _masterRenderer.handleWindowResize(_swapchain);
            _window._resized = false;
        }
        else
        {
            _inputManager.waitEvents();
        }
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
