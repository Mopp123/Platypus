#include "platypus/core/Window.hpp"
#include "DesktopWindow.hpp"
#include "platypus/core/Debug.hpp"
#include <GLFW/glfw3.h>


namespace platypus
{
    Window::Window(
        const std::string& title,
        int width,
        int height,
        bool resizable,
        WindowMode mode
    ) :
        _width(width),
        _height(height),
        _mode(mode)
    {
        if (mode != WindowMode::WINDOWED)
        {
            Debug::log(
                "@Window::Window "
                "Current desktop window implementation allows windows to be "
                "only in windowed mode",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

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

        // Hyprland issues...
        // TODO: Figure out some way to create resizable floating windows in Hyprland...
        bool floatingStaticWindow = !resizable && (mode != WindowMode::FULLSCREEN);
        if (floatingStaticWindow)
            glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // TODO: Fullscreen support?
        GLFWwindow* pGLFWwindow = glfwCreateWindow(
            _width,
            _height,
            title.c_str(),
            mode == WindowMode::FULLSCREEN ? glfwGetPrimaryMonitor() : NULL,
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

        // If non resizable floating window, at least on Hyprland, need to set the
        // window size after creation or fucks up the scale...
        // TODO: Test with other window managers!
        if (floatingStaticWindow)
        {
            glfwSetWindowSizeLimits(pGLFWwindow, _width, _height, _width, _height);
            glfwSetWindowSize(pGLFWwindow, _width, _height);
        }

        _pImpl = new WindowImpl;
        _pImpl->pGLFWwindow = pGLFWwindow;

        Debug::log("Window created");
    }

    Window::~Window()
    {
        if (_pImpl->surface)
        {
            Debug::log(
                "@Window::~Window "
                "Window surface not destroyed. "
                "Context is responsible for creating and destroying the window surface. "
                "Context::destroy() should be called before window's destruction!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_pImpl->pGLFWwindow)
        {
            glfwDestroyWindow(_pImpl->pGLFWwindow);
        }
        glfwTerminate();
        delete _pImpl;
    }

    void Window::requestClosing()
    {
        glfwSetWindowShouldClose(_pImpl->pGLFWwindow, 1);
    }

    bool Window::isCloseRequested()
    {
        return (bool)glfwWindowShouldClose(_pImpl->pGLFWwindow);
    }

    void Window::getSurfaceExtent(int* pWidth, int* pHeight) const
    {
        glfwGetFramebufferSize(_pImpl->pGLFWwindow, pWidth, pHeight);
    }

    WindowImpl* Window::getImpl()
    {
        return _pImpl;
    }

    const WindowImpl* Window::getImpl() const
    {
        return _pImpl;
    }
}
