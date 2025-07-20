#include "platypus/core/Window.hpp"
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

    // Quite dumb, but couldn't figure out quickly enough how
    // to get references or ptrs working with this kind of stuff
    EM_JS(int, get_canvas_width, (), {
        var c = document.getElementById('canvas');
        return c.width;
    });

    EM_JS(int, get_canvas_height, (), {
        var c = document.getElementById('canvas');
        return c.height;
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
      WindowMode mode
    ) :
        _width(width),
        _height(height),
        _mode(mode)
    {
        if (mode == WindowMode::FULLSCREEN)
        {
            Debug::log(
                "@Window::Window "
                "Current web window implementation doesn't support fullscreen yet",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        else if (mode == WindowMode::WINDOWED)
        {
            resize_canvas(width, height);
        }
        else if (mode == WindowMode::WINDOWED_FIT_SCREEN)
        {
            fit_page();
        }

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
        *pWidth = get_canvas_width();
        *pHeight = get_canvas_height();
    }

    WindowImpl* Window::getImpl()
    {
        return _pImpl;
    }
}
