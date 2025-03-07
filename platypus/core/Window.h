#pragma once

#include <string>


namespace platypus
{
    // @WINDOWED_FIT_SCREEN:
    //  On desktop:
    //      Puts to screen resolution but on windowed mode
    //  On web:
    //      Puts the canvas to fit the browser window's "inner scale" and
    //      resizes if appropriately
    //
    enum class WindowMode
    {
        WINDOWED,
        WINDOWED_FIT_SCREEN,
        FULLSCREEN
    };

    class InputManager;

    struct WindowImpl;
    class Window
    {
    private:
        friend class InputManager;
        int _width = 800;
        int _height = 600;
        bool _resized = false;
        WindowMode _mode;

        WindowImpl* _pImpl = nullptr;

    public:
        Window(
            const std::string& title,
            int width,
            int height,
            bool resizable,
            WindowMode mode
        );
        ~Window();

        bool isCloseRequested();

        void getSurfaceExtent(int* pWidth, int* pHeight) const;

        WindowImpl* getImpl();

        // @handleWindowResizeEvent:
        //  *Needed to have some way to handle window resizing outside of regular events.
        //  Sets new window dimensions and flags it as resized.
        inline void resize(int width, int height)
        {
            _width = width;
            _height = height;
            _resized = true;
        }

        inline int getWidth() const { return _width; }
        inline int getHeight() const { return _height; }
        inline bool resized() const { return _resized; }
        inline bool isMinimized() const { return _width == 0 || _height == 0; }
        inline void resetResized() { _resized = false; }
        inline WindowMode getMode() const { return _mode; }
    };
}
