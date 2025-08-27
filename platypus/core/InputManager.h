#pragma once

#include "InputEvent.h"
#include "platypus/core/Window.hpp"
#include <unordered_map>
#include <vector>


namespace platypus
{
    /*
        Some common, platform agnostic behaviour is implemented in platypus/core/InputManager.cpp
        while platform specific implementation can be found from platypus/core/platform/<PLATFORM>

        Member functions requiring platform specific implementations:
        * Constructor
        * Destructor
        * pollEvents()
    */
    class InputManager
    {
    private:
        std::vector<std::pair<KeyEvent*, void(KeyEvent::*)(KeyName, int, InputAction, int)>>                        _keyEvents;
        std::vector<std::pair<MouseButtonEvent*, void(MouseButtonEvent::*)(MouseButtonName, InputAction, int)>>     _mouseButtonEvents;
        std::vector<std::pair<CursorPosEvent*, void(CursorPosEvent::*)(int, int)>>                                  _cursorPosEvents;
        std::vector<std::pair<ScrollEvent*, void(ScrollEvent::*)(double, double)>>                                  _scrollEvents;
        std::vector<std::pair<CharInputEvent*, void(CharInputEvent::*)(unsigned int codepoint)>>                    _charInputEvents;
        std::vector<std::pair<WindowResizeEvent*, void(WindowResizeEvent::*)(int, int)>>                            _windowResizeEvent;

        int _mouseX = 0;
        int _mouseY = 0;

        Window& _windowRef;

    public:
        // Mainly for testing purposes!
        // To check immediate key and mouse down instead of having to always have some inputEvent
        std::unordered_map<KeyName, bool> _keyDown;
        std::unordered_map<MouseButtonName, bool> _mouseDown;

    public:
        InputManager(Window& windowRef);
        InputManager(const InputManager&) = delete;
        ~InputManager();

        // Following functions require platform specific implementation.
        // @handleWindowResizeEvent:
        //  * Needed to have some way to handle window resizing outside of regular events.
        //  * Needs to at least set window dimensions and flag it as resized
        //  * Call all necessary platform independent window resizing stuff
        void handleWindowResizing(int width, int height);
        void pollEvents();
        void waitEvents();

        // All below implemented by core/InputManager.cpp
        void addKeyEvent(KeyEvent* ev);
        void addMouseButtonEvent(MouseButtonEvent* ev);
        void addCursorPosEvent(CursorPosEvent* ev);
        void addScrollEvent(ScrollEvent* ev);
        void addCharInputEvent(CharInputEvent* ev);
        void addWindowResizeEvent(WindowResizeEvent* ev);

        void destroyEvents();

        void processKeyEvents(KeyName key, int scancode, InputAction action, int mods);
        void processMouseButtonEvents(MouseButtonName button, InputAction action, int mods);
        void processCursorPosEvents(int x, int y);
        void processScrollEvents(double dx, double dy);
        void processCharInputEvents(unsigned int codepoint);
        void processWindowResizeEvents(int w, int h);

        bool isKeyDown(KeyName key) const;
        bool isMouseButtonDown(MouseButtonName button) const;

        inline void setMousePos(int x, int y) { _mouseX = x; _mouseY = y; }
        inline int getMouseX() const { return _mouseX; }
        inline int getMouseY() const { return _mouseY; }
    };
}
