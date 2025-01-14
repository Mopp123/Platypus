#include "Application.h"
#include "Debug.h"


namespace platypus
{
    Application* Application::s_pInstance = nullptr;

    Application::Application(Window* pWindow, InputManager* pInputManager) :
        _pWindow(pWindow),
        _pInputManager(pInputManager)
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
        while (!_pWindow->isCloseRequested())
        {
            _pInputManager->pollEvents();
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
