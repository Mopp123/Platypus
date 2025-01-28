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
        bool fullscreen
    ) :
        _window(name, width, height, resizable, fullscreen),
        _inputManager(&_window),
        _context(name.c_str(), &_window),
        _swapchain(_window),
        // NOTE: Not sure is CommanPool created at this point -> fucks up master renderer creation if not!
        _masterRenderer(_commandPool)
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

        _masterRenderer.allocCommandBuffers(_swapchain.getImageCount());
        _masterRenderer.createPipelines(_swapchain);
    }

    Application::~Application()
    {
    }

    static size_t s_framesInFlight;
    static size_t s_currentFrame = 0;
    void Application::run()
    {
        s_framesInFlight = _swapchain.getMaxFramesInFlight();
        while (!_window.isCloseRequested())
        {
            _inputManager.pollEvents();

            // NOTE: Below should probably be done somewhere else...
            AcquireSwapchainImageResult result = _swapchain.acquireImage();
            if (result == AcquireSwapchainImageResult::ERROR)
            {
                Debug::log(
                    "@Application::run "
                    "Failed to acquire swapchain image!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            else if (result == AcquireSwapchainImageResult::RESIZE_REQUIRED)
            {
                Debug::log(
                    "@Application::run "
                    "Failed to acquire swapchain image! "
                    "Window resize may have happened. Currently this isn't handled!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }
            else
            {
                const CommandBuffer& cmdBuf = _masterRenderer.recordCommandBuffer(_swapchain, s_currentFrame);
                _context.submitPrimaryCommandBuffer(_swapchain, cmdBuf, _swapchain.getCurrentFrame());
                _swapchain.present(_swapchain.getCurrentFrame());

                s_currentFrame = (s_currentFrame + 1) % s_framesInFlight;
            }
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
