#pragma once


namespace platypus
{
    enum class KeyName
    {
        KEY_NOT_FOUND,

        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,

        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,

        KEY_Q,
        KEY_W,
        KEY_E,
        KEY_R,
        KEY_T,
        KEY_Y,
        KEY_U,
        KEY_I,
        KEY_O,
        KEY_P,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_Z,
        KEY_X,
        KEY_C,
        KEY_V,
        KEY_B,
        KEY_N,
        KEY_M,

        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,

        KEY_SPACE,
        KEY_BACKSPACE,
        KEY_ENTER,
        KEY_LCTRL,
        KEY_SHIFT,
        KEY_TAB,
        KEY_ESCAPE
    };

    enum class MouseButtonName
    {
        MOUSE_BUTTON_NOT_FOUND,
        MOUSE_LEFT,
        MOUSE_MIDDLE,
        MOUSE_RIGHT
    };

    enum class InputAction
    {
        ACTION_NOT_FOUND,
        PRESS,
        RELEASE
    };

    class KeyEvent
    {
    public:
        virtual ~KeyEvent() {};
        virtual void func(KeyName key, int scancode, InputAction action, int mods) = 0;
    };

    class MouseButtonEvent
    {
    public:
        virtual ~MouseButtonEvent() {};
        virtual void func(MouseButtonName button, InputAction action, int mods) = 0;
    };

    class CursorPosEvent
    {
    public:
        virtual ~CursorPosEvent() {};
        virtual void func(int x, int y) = 0;
    };

    class ScrollEvent
    {
    public:
        virtual ~ScrollEvent() {};
        virtual void func(double xOffset, double yOffset) = 0;
    };

    class CharInputEvent
    {
    public:
        virtual ~CharInputEvent() {};
        virtual void func(unsigned int codepoint) = 0;
    };
    class WindowResizeEvent
    {
    public:
        virtual ~WindowResizeEvent() {};
        virtual void func(int w, int h) = 0;
    };
}
