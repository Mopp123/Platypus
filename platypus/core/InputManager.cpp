#include "InputManager.hpp"


namespace platypus
{
    void InputManager::addKeyEvent(KeyEvent* keyEvent)
    {
        _keyEvents.push_back(std::make_pair(keyEvent, &KeyEvent::func));
    }
    void InputManager::addMouseButtonEvent(MouseButtonEvent* ev)
    {
        _mouseButtonEvents.push_back(std::make_pair(ev, &MouseButtonEvent::func));
    }
    void InputManager::addCursorPosEvent(CursorPosEvent* ev)
    {
        _cursorPosEvents.push_back(std::make_pair(ev, &CursorPosEvent::func));
    }
    void InputManager::addScrollEvent(ScrollEvent* ev)
    {
        _scrollEvents.push_back(std::make_pair(ev, &ScrollEvent::func));
    }
    void InputManager::addCharInputEvent(CharInputEvent* ev)
    {
        _charInputEvents.push_back(std::make_pair(ev, &CharInputEvent::func));
    }
    void InputManager::addWindowResizeEvent(WindowResizeEvent* ev)
    {
        _windowResizeEvent.push_back(std::make_pair(ev, &WindowResizeEvent::func));
    }


    void InputManager::destroyEvents()
    {
        for (std::pair<KeyEvent*, void(KeyEvent::*)(KeyName, int, InputAction, int)>& ev : _keyEvents)
            delete ev.first;

        for (std::pair<MouseButtonEvent*, void(MouseButtonEvent::*)(MouseButtonName, InputAction, int)>& ev : _mouseButtonEvents)
            delete ev.first;

        for (std::pair<CursorPosEvent*, void(CursorPosEvent::*)(int, int)>& ev : _cursorPosEvents)
            delete ev.first;

        for (std::pair<ScrollEvent*, void(ScrollEvent::*)(double, double)>& ev : _scrollEvents)
            delete ev.first;

        for (std::pair<CharInputEvent*, void(CharInputEvent::*)(unsigned int)>& ev : _charInputEvents)
            delete ev.first;

        for (std::pair<WindowResizeEvent*, void(WindowResizeEvent::*)(int, int)>& ev : _windowResizeEvent)
            delete ev.first;

        _keyEvents.clear();
        _mouseButtonEvents.clear();
        _cursorPosEvents.clear();
        _scrollEvents.clear();
        _charInputEvents.clear();
        _windowResizeEvent.clear();
    }


    void InputManager::processKeyEvents(KeyName key, int scancode, InputAction action, int mods)
    {
        for (std::pair<KeyEvent*, void(KeyEvent::*)(KeyName, int, InputAction, int)>& ev : _keyEvents)
        {
            KeyEvent* caller = ev.first;
            void(KeyEvent:: * eventFunc)(KeyName, int, InputAction, int) = ev.second;
            (caller->*eventFunc)(key, scancode, action, mods);
        }
    }

    void InputManager::processMouseButtonEvents(MouseButtonName button, InputAction action, int mods)
    {
        for (std::pair<MouseButtonEvent*, void(MouseButtonEvent::*)(MouseButtonName, InputAction, int)>& ev : _mouseButtonEvents)
        {
            MouseButtonEvent* caller = ev.first;
            void(MouseButtonEvent:: * eventFunc)(MouseButtonName, InputAction, int) = ev.second;
            (caller->*eventFunc)(button, action, mods);
        }
    }

    void InputManager::processCursorPosEvents(int mx, int my)
    {
        for (std::pair<CursorPosEvent*, void(CursorPosEvent::*)(int, int)>& ev : _cursorPosEvents)
        {
            CursorPosEvent* caller = ev.first;
            void(CursorPosEvent:: * eventFunc)(int, int) = ev.second;
            (caller->*eventFunc)(mx, my);
        }
    }

    void InputManager::processScrollEvents(double dx, double dy)
    {
        for (std::pair<ScrollEvent*, void(ScrollEvent::*)(double, double)>& ev : _scrollEvents)
        {
            ScrollEvent* caller = ev.first;
            void(ScrollEvent:: * eventFunc)(double, double) = ev.second;
            (caller->*eventFunc)(dx, dy);
        }
    }


    void InputManager::processCharInputEvents(unsigned int codepoint)
    {
        for (std::pair<CharInputEvent*, void(CharInputEvent::*)(unsigned int)>& ev : _charInputEvents)
        {
            CharInputEvent* caller = ev.first;
            void(CharInputEvent:: * eventFunc)(unsigned int) = ev.second;
            (caller->*eventFunc)(codepoint);
        }
    }

    void InputManager::processWindowResizeEvents(int w, int h)
    {
        for (std::pair<WindowResizeEvent*, void(WindowResizeEvent::*)(int, int)>& ev : _windowResizeEvent)
        {
            WindowResizeEvent* caller = ev.first;
            void(WindowResizeEvent:: * eventFunc)(int, int) = ev.second;
            (caller->*eventFunc)(w, h);
        }
    }

    bool InputManager::isKeyDown(KeyName key) const
    {
        std::unordered_map<KeyName, bool>::const_iterator it = _keyDown.find(key);
        if (it != _keyDown.end())
            return it->second;
        return false;
    }

    bool InputManager::isMouseButtonDown(MouseButtonName button) const
    {
        std::unordered_map<MouseButtonName, bool>::const_iterator it = _mouseDown.find(button);
        if (it != _mouseDown.end())
            return it->second;
        return false;
    }
}
