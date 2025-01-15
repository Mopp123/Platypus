#include "platypus/core/Window.h"
#include "platypus/core/Debug.h"
#include <GLFW/glfw3.h>


namespace platypus
{
    struct WindowImpl
    {
        GLFWwindow* pGLFWwindow = nullptr;
    };


    Window::Window(
        const std::string& title,
        int width,
        int height,
        bool resizable,
        bool fullscreen
    ) :
        _width(width),
        _height(height)
    {
        if (!glfwInit())
        {
            const char* errDescription;
            int glfwErrCode = glfwGetError(&errDescription);
            std::string errDescriptionStr(errDescription);
            Debug::log(
                "@Window::Window "
                "glfwInit() failed with error code: " + std::to_string(glfwErrCode) + " "
                "description: " + errDescriptionStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // TODO: Fullscreen support?
        GLFWwindow* pGLFWwindow = glfwCreateWindow(
            _width,
            _height,
            title.c_str(),
            fullscreen ? glfwGetPrimaryMonitor() : NULL,
            NULL
        );
        if (!pGLFWwindow)
        {
            const char* errDescription;
            int glfwErrCode = glfwGetError(&errDescription);
            std::string errDescriptionStr(errDescription);
            Debug::log(
                "@Window::Window "
                "glfwCreateWindow failed with error code: " + std::to_string(glfwErrCode) + " "
                "description: " + errDescriptionStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            glfwTerminate();
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new WindowImpl;
        _pImpl->pGLFWwindow = pGLFWwindow;

        Debug::log("Window created");
    }

    Window::~Window()
    {
        if (_pImpl->pGLFWwindow)
            glfwDestroyWindow(_pImpl->pGLFWwindow);
        glfwTerminate();
        delete _pImpl;
    }

    bool Window::isCloseRequested()
    {
        return (bool)glfwWindowShouldClose(_pImpl->pGLFWwindow);
    }

    void* Window::getWindowHandle()
    {
        return _pImpl->pGLFWwindow;
    }
}
