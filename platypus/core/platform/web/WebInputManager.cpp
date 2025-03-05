#include "platypus/core/InputEvent.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/Debug.h"

namespace platypus
{
    const std::unordered_map<std::string, InputKeyName> s_emscToKeyMapping
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


    const std::unordered_map<unsigned short, InputMouseButtonName> s_emscToMouseButtonMapping
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


    EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        if (s_emscToKeyMapping.find(keyEvent->key) == s_emscToKeyMapping.end())
        {
            Debug::log(
                "@keydown_callback "
                "Failed to find key: " + std::string(keyEvent->key),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return true;
        }

        KeyName keyName = s_emscToKeyMapping[keyEvent->key];
        pInputManager->_keyDown[keyName] = true;
        // NOTE: Don't know yet what we should do with scancodes and mods here... if anything...
        int scancode = 0;
        int mods = 0;
        pInputManager->processKeyEvents(keyName, scancode, InputAction::PRESS, mods);

        // check is this just a 'char' -> process char input events
        if (inputManager->is_character(keyEvent->key))
        {
            unsigned char b1 = keyEvent->key[0];
            //unsigned char b2 = keyEvent->key[1];
            //unsigned int codepoint = inputManager->parseSpecialCharCodepoint(b2 == 0 ? (unsigned int)b1 : (unsigned int)b2);

            // Extended chars are disabled atm!
            unsigned int codepoint = b1;

            inputManager->processCharInputEvents(codepoint);
        }

        return true;
    }
    EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        if (s_emscToKeyMapping.find(keyEvent->key) == s_emscToKeyMapping.end())
        {
            Debug::log(
                "@keyup_callback "
                "Failed to find key: " + std::string(keyEvent->key),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return true;
        }

        KeyName keyName = s_emscToKeyMapping[keyEvent->key];
        pInputManager->_keyDown[keyName] = false;
        // NOTE: Don't know yet what we should do with scancodes and mods here... if anything...
        int scancode = 0;
        int mods = 0;
        pInputManager->processKeyEvents(keyName, scancode, InputAction::RELEASE, mods);

        return true;
    }

    EM_BOOL mouse_down_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        if (s_emscToMouseButtonMapping.find(mouseEvent->button) == s_emscToMouseButtonMapping.end())
        {
            Debug::log(
                "@mouse_down_callback "
                "Failed to find button: " + std::string(mouseEvent->button),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return true;
        }

        MouseButtonName buttonName = s_emscToMouseButtonMapping[mouseEvent->button];
        pInputManager->_mouseDown[buttonName] = true;
        int mods = 0;
        pInputManager->processMouseButtonEvents(buttonName, InputAction::PRESS, mods);

        return true;
    }
    EM_BOOL mouse_up_callback(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        InputManager* pInputManager = (InputManager*)userData;

        if (s_emscToMouseButtonMapping.find(mouseEvent->button) == s_emscToMouseButtonMapping.end())
        {
            Debug::log(
                "@mouse_up_callback "
                "Failed to find button: " + std::string(mouseEvent->button),
                Debug::MessageType::PLATYPUS_ERROR
            );
            return true;
        }

        MouseButtonName buttonName = s_emscToMouseButtonMapping[mouseEvent->button];
        pInputManager->_mouseDown[buttonName] = false;
        int mods = 0;
        pInputManager->processMouseButtonEvents(buttonName, InputAction::RELEASE, mods);

        return true;
    }

    InputManager::InputManager(Window& windowRef)
    {
    }

    InputManager::~InputManager()
    {
    }

    void InputManager::handleWindowResizeEvent(int width, int height)
    {
    }

    void InputManager::pollEvents()
    {
    }

    void InputManager::waitEvents()
    {
    }
}
