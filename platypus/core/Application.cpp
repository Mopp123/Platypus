#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"


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
        _swapchain(_window, _context)
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
    }

    Application::~Application()
    {
    }

    void Application::run()
    {
        while (!_window.isCloseRequested())
        {
            _inputManager.pollEvents();
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
