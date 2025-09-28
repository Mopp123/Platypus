#include "platypus/core/InputEvent.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"

#include <emscripten.h>
#include <emscripten/em_types.h>
#include <emscripten/html5.h>


namespace platypus
{
    // Quick hack to have mouse move stuff work
    static bool s_isMouseMoving = false;

    static const std::unordered_map<std::string, KeyName> s_emscToKeyMapping
    {
        { "0", KeyName::KEY_0 },
        { "1", KeyName::KEY_1 },
        { "2", KeyName::KEY_2 },
        { "3", KeyName::KEY_3 },
        { "4", KeyName::KEY_4 },
        { "5", KeyName::KEY_5 },
        { "6", KeyName::KEY_6 },
        { "7", KeyName::KEY_7 },
        { "8", KeyName::KEY_8 },
        { "9", KeyName::KEY_9 },

        {"F1",  KeyName::KEY_F1 },
        {"F2",  KeyName::KEY_F2 },
        {"F3",  KeyName::KEY_F3 },
        {"F4",  KeyName::KEY_F4 },
        {"F5",  KeyName::KEY_F5 },
        {"F6",  KeyName::KEY_F6 },
        {"F7",  KeyName::KEY_F7 },
        {"F8",  KeyName::KEY_F8 },
        {"F9",  KeyName::KEY_F9 },
        {"F10", KeyName::KEY_F10 },
        {"F11", KeyName::KEY_F11 },
        {"F12", KeyName::KEY_F12 },

        { "q", KeyName::KEY_Q },
        { "w", KeyName::KEY_W },
        { "e", KeyName::KEY_E },
        { "r", KeyName::KEY_R },
        { "t", KeyName::KEY_T },
        { "y", KeyName::KEY_Y },
        { "u", KeyName::KEY_U },
        { "i", KeyName::KEY_I },
        { "o", KeyName::KEY_O },
        { "p", KeyName::KEY_P },
        { "a", KeyName::KEY_A },
        { "s", KeyName::KEY_S },
        { "d", KeyName::KEY_D },
        { "f", KeyName::KEY_F },
        { "g", KeyName::KEY_G },
        { "h", KeyName::KEY_H },
        { "j", KeyName::KEY_J },
        { "k", KeyName::KEY_K },
        { "l", KeyName::KEY_L },
        { "z", KeyName::KEY_Z },
        { "x", KeyName::KEY_X },
        { "c", KeyName::KEY_C },
        { "v", KeyName::KEY_V },
        { "b", KeyName::KEY_B },
        { "n", KeyName::KEY_N },
        { "m", KeyName::KEY_M },

        { "ArrowUp",      KeyName::KEY_UP    },
        { "ArrowDown",    KeyName::KEY_DOWN  },
        { "ArrowLeft",    KeyName::KEY_LEFT  },
        { "ArrowRight",   KeyName::KEY_RIGHT },

        { " ",           KeyName::KEY_SPACE     },
        { "Backspace",   KeyName::KEY_BACKSPACE },
        { "Enter",       KeyName::KEY_ENTER     },
        { "Control",     KeyName::KEY_LCTRL     },
        { "Shift",       KeyName::KEY_SHIFT     },
        { "Tab",         KeyName::KEY_TAB       },
        { "Escape",      KeyName::KEY_ESCAPE    }
    };


    static const std::unordered_map<unsigned short, MouseButtonName> s_emscToMouseButtonMapping
    {
        { 0, MouseButtonName::MOUSE_LEFT   },
        { 1, MouseButtonName::MOUSE_RIGHT  },
        { 2, MouseButtonName::MOUSE_MIDDLE }
    };


    static bool is_character(const char* keyname)
    {
        auto iter = s_emscToKeyMapping.find(keyname);
        if (iter != s_emscToKeyMapping.end())
        {
            const KeyName& k = iter->second;
            return k != KeyName::KEY_BACKSPACE && k != KeyName::KEY_ENTER && k != KeyName::KEY_SHIFT && k != KeyName::KEY_LCTRL;
        }
        return true;
    }


    EM_JS(int, webwindow_get_width, (), {
        return window.width;
    });

    EM_JS(int, webwindow_get_height, (), {
        return window.height;
    });

    EM_JS(int, webwindow_get_inner_width, (), {
        return window.innerWidth;
    });

    EM_JS(int, webwindow_get_inner_height, (), {
       return window.innerHeight;
    });


    static EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;
        std::unordered_map<std::string, KeyName>::const_iterator keyIt = s_emscToKeyMapping.find(keyEvent->key);
        if (keyIt == s_emscToKeyMapping.end())
        {
            Debug::log(
                "@keydown_callback "
                "Failed to find key: " + std::string(keyEvent->key),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return EMSCRIPTEN_RESULT_SUCCESS;
        }

        KeyName keyName = keyIt->second;
        pInputManager->_keyDown[keyName] = true;
        // NOTE: Don't know yet what we should do with scancodes and mods here... if anything...
        int scancode = 0;
        int mods = 0;
        pInputManager->processKeyEvents(keyName, scancode, InputAction::PRESS, mods);

        // check is this just a 'char' -> process char input events
        if (is_character(keyEvent->key))
        {
            unsigned char b1 = keyEvent->key[0];
            //unsigned char b2 = keyEvent->key[1];
            //unsigned int codepoint = inputManager->parseSpecialCharCodepoint(b2 == 0 ? (unsigned int)b1 : (unsigned int)b2);

            // Extended chars are disabled atm!
            unsigned int codepoint = b1;

            pInputManager->processCharInputEvents(codepoint);
        }

        return EMSCRIPTEN_RESULT_SUCCESS;
    }

    static EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;
        std::unordered_map<std::string, KeyName>::const_iterator keyIt = s_emscToKeyMapping.find(keyEvent->key);
        if (keyIt == s_emscToKeyMapping.end())
        {
            Debug::log(
                "@keyup_callback "
                "Failed to find key: " + std::string(keyEvent->key),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return 0;
        }

        KeyName keyName = keyIt->second;
        pInputManager->_keyDown[keyName] = false;
        // NOTE: Don't know yet what we should do with scancodes and mods here... if anything...
        int scancode = 0;
        int mods = 0;
        pInputManager->processKeyEvents(keyName, scancode, InputAction::RELEASE, mods);

        return 0;
    }

    static EM_BOOL mouse_down_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;
        std::unordered_map<unsigned short, MouseButtonName>::const_iterator buttonIt = s_emscToMouseButtonMapping.find(mouseEvent->button);
        if (buttonIt == s_emscToMouseButtonMapping.end())
        {
            Debug::log(
                "@mouse_down_callback "
                "Failed to find button: " + std::to_string(mouseEvent->button),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return 0;
        }

        MouseButtonName buttonName = buttonIt->second;
        pInputManager->_mouseDown[buttonName] = true;
        int mods = 0;
        pInputManager->processMouseButtonEvents(buttonName, InputAction::PRESS, mods);

        return 0;
    }

    static EM_BOOL mouse_up_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;
        std::unordered_map<unsigned short, MouseButtonName>::const_iterator buttonIt = s_emscToMouseButtonMapping.find(mouseEvent->button);
        if (buttonIt == s_emscToMouseButtonMapping.end())
        {
            Debug::log(
                "@mouse_up_callback "
                "Failed to find button: " + std::to_string(mouseEvent->button),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return 0;
        }

        MouseButtonName buttonName = buttonIt->second;
        pInputManager->_mouseDown[buttonName] = false;
        int mods = 0;
        pInputManager->processMouseButtonEvents(buttonName, InputAction::RELEASE, mods);

        return 0;
    }

    static EM_BOOL cursor_pos_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        // In web gl our coords are flipped -> need to flip mouseY
        // NOTE: ATM DISABLED ONLY FOR TESTING
        //const int windowHeight = Application::get_instance()->getWindow().getHeight();

        int mx = mouseEvent->targetX;
        int my = mouseEvent->targetY;
        //int my = windowHeight - mouseEvent->targetY;
        pInputManager->setMousePos(mx, my);
        pInputManager->processCursorPosEvents(mx, my);

        return 0;
    }

    static EM_BOOL scroll_callback(int eventType, const EmscriptenWheelEvent* wheelEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        double scroll = wheelEvent->deltaY;
        // Maybe take deltaX into account too eventually
        pInputManager->processScrollEvents(0, scroll);

        return 0;
    }

    // NOTE: On web platform "window" resizing works differently from desktop.
    // We can only detect if the actual browser window is resized.
    // Canvas in which we render can't get resized UNLESS in following cases:
    //  * The application has some configurable settings where user can specify resolution
    //  * The application has been setup to always fit the canvas to the inner scale of the browser
    //  window.
    static EM_BOOL ui_callback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData)
    {
        Debug::log("___TEST___WARNING UI CALLBACK!");
        InputManager* pInputManager = (InputManager*)userData;
        const Window& window = Application::get_instance()->getWindow();
        if (window.getMode() == WindowMode::WINDOWED_FIT_SCREEN && eventType == EMSCRIPTEN_EVENT_RESIZE)
        {
            int width = webwindow_get_width();
            int height = webwindow_get_height();
            int surfaceWidth = webwindow_get_inner_width();
            int surfaceHeight = webwindow_get_inner_height();

            // NOTE: The passed width, height doesn't do anything here
            pInputManager->handleWindowResizing(width, height);

            int actualWidth = 0;
            int actualHeight = 0;
            window.getSurfaceExtent(&actualWidth, &actualHeight);

            pInputManager->processWindowResizeEvents(actualWidth, actualHeight);
        }

        return 0;
    }


    InputManager::InputManager(Window& windowRef) :
        _windowRef(windowRef)
    {
        //emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, key_callback);
        //emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, keypress_callback);

        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, keydown_callback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, keyup_callback);

        emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, mouse_down_callback);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, mouse_up_callback);

        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, cursor_pos_callback);
        emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, scroll_callback);


        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, ui_callback);
    }

    InputManager::~InputManager()
    {
    }

    void InputManager::handleWindowResizing(int width, int height)
    {
        if (_windowRef.getMode() == WindowMode::WINDOWED_FIT_SCREEN)
        {
            // "Fit canvas to window inner scale"
            EM_ASM({
                var c = document.getElementById('canvas');
                c.width = window.innerWidth;
                c.height = window.innerHeight;
            });

            int actualWidth = 0;
            int actualHeight = 0;
            _windowRef.getSurfaceExtent(&actualWidth, &actualHeight);
            _windowRef._width = actualWidth;
            _windowRef._height = actualHeight;
        }
        else
        {
            _windowRef._width = width;
            _windowRef._height = height;
        }

        _windowRef._resized = true;
    }

    void InputManager::pollEvents()
    {
    }

    void InputManager::waitEvents()
    {
    }
}
