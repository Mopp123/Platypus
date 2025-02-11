#pragma once

#include <string>


namespace platypus
{
    class InputManager;

    struct WindowImpl;
    class Window
    {
    private:
        friend class InputManager;
        int _width = 800;
        int _height = 600;
        bool _resized = false;

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

        void getSurfaceExtent(int* pWidth, int* pHeight) const;

        WindowImpl* getImpl();

        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }
        inline bool resized() const { return _resized; }
        inline bool isMinimized() const { return _width == 0 || _height == 0; }
        inline void resetResized() { _resized = false; }
    };
}
