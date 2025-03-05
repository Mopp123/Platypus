#include "platypus/core/Window.h"
#include "platypus/Common.h"
#include "platypus/core/Debug.h"

#include <emscripten.h>
#include <emscripten/html5.h>


namespace platypus
{
    struct WindowImpl
    {
    };


    EM_JS(void, resize_canvas, (int w, int h), {
        var c = document.getElementById('canvas');
        c.width = w;
        c.height = h;
    });


    EM_JS(void, get_canvas_scale, (Extent2D& extent), {
        var c = document.getElementById('canvas');
        extent.width = c.width;
        extent.height = c.height;
    });


    EM_JS(void, fit_page, (), {
        var c = document.getElementById('canvas');
        c.width  = window.innerWidth;
        c.height = window.innerHeight;
    });


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
        if (fullscreen)
        {
            Debug::log(
                "@Window::Window "
                "Requested fullscreen window but this feature isn't available yet",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        resize_canvas(width, height);
        _pImpl = new WindowImpl;

        Debug::log("Window created successfully");
    }

    Window::~Window()
    {
        if (_pImpl)
            delete _pImpl;
    }

    bool Window::isCloseRequested()
    {
        return false;
    }

    void Window::getSurfaceExtent(int* pWidth, int* pHeight) const
    {
        Extent2D canvasScale{};
        get_canvas_scale(canvasScale);
        *pWidth = canvasScale.width;
        *pHeight = canvasScale.height;
    }

    WindowImpl* Window::getImpl()
    {
        return _pImpl;
    }
}
