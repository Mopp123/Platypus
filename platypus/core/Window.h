#pragma once

#include <string>


namespace platypus
{
    struct WindowImpl;

    class Window
    {
    private:
        int _width = 800;
        int _height = 600;

        WindowImpl* _pImpl = nullptr;

    public:
        Window(
            const std::string& title,
            int width,
            int height,
            bool resizable,
            bool fullscreen
        );
        ~Window();

        bool isCloseRequested();

        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }

        void* getWindowHandle();
        void* getImpl();
    };
}
