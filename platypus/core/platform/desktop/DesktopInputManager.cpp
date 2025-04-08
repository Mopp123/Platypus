#include "platypus/core/InputManager.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include "DesktopWindow.h"
#include <GLFW/glfw3.h>


namespace platypus
{
    static std::unordered_map<int, KeyName> s_glfwToKeyMapping = {
        { GLFW_KEY_0, KeyName::KEY_0 },
        { GLFW_KEY_1, KeyName::KEY_1 },
        { GLFW_KEY_2, KeyName::KEY_2 },
        { GLFW_KEY_3, KeyName::KEY_3 },
        { GLFW_KEY_4, KeyName::KEY_4 },
        { GLFW_KEY_5, KeyName::KEY_5 },
        { GLFW_KEY_6, KeyName::KEY_6 },
        { GLFW_KEY_7, KeyName::KEY_7 },
        { GLFW_KEY_8, KeyName::KEY_8 },
        { GLFW_KEY_9, KeyName::KEY_9 },

        { GLFW_KEY_F1,  KeyName::KEY_F1 },
        { GLFW_KEY_F2,  KeyName::KEY_F2 },
        { GLFW_KEY_F3,  KeyName::KEY_F3 },
        { GLFW_KEY_F4,  KeyName::KEY_F4 },
        { GLFW_KEY_F5,  KeyName::KEY_F5 },
        { GLFW_KEY_F6,  KeyName::KEY_F6 },
        { GLFW_KEY_F7,  KeyName::KEY_F7 },
        { GLFW_KEY_F8,  KeyName::KEY_F8 },
        { GLFW_KEY_F9,  KeyName::KEY_F9 },
        { GLFW_KEY_F10, KeyName::KEY_F10 },
        { GLFW_KEY_F11, KeyName::KEY_F11 },
        { GLFW_KEY_F12, KeyName::KEY_F12 },

        { GLFW_KEY_Q, KeyName::KEY_Q },
        { GLFW_KEY_W, KeyName::KEY_W },
        { GLFW_KEY_E, KeyName::KEY_E },
        { GLFW_KEY_R, KeyName::KEY_R },
        { GLFW_KEY_T, KeyName::KEY_T },
        { GLFW_KEY_Y, KeyName::KEY_Y },
        { GLFW_KEY_U, KeyName::KEY_U },
        { GLFW_KEY_I, KeyName::KEY_I },
        { GLFW_KEY_O, KeyName::KEY_O },
        { GLFW_KEY_P, KeyName::KEY_P },
        { GLFW_KEY_A, KeyName::KEY_A },
        { GLFW_KEY_S, KeyName::KEY_S },
        { GLFW_KEY_D, KeyName::KEY_D },
        { GLFW_KEY_F, KeyName::KEY_F },
        { GLFW_KEY_G, KeyName::KEY_G },
        { GLFW_KEY_H, KeyName::KEY_H },
        { GLFW_KEY_J, KeyName::KEY_J },
        { GLFW_KEY_K, KeyName::KEY_K },
        { GLFW_KEY_L, KeyName::KEY_L },
        { GLFW_KEY_Z, KeyName::KEY_Z },
        { GLFW_KEY_X, KeyName::KEY_X },
        { GLFW_KEY_C, KeyName::KEY_C },
        { GLFW_KEY_V, KeyName::KEY_V },
        { GLFW_KEY_B, KeyName::KEY_B },
        { GLFW_KEY_N, KeyName::KEY_N },
        { GLFW_KEY_M, KeyName::KEY_M },

        { GLFW_KEY_UP,    KeyName::KEY_UP   },
        { GLFW_KEY_DOWN,  KeyName::KEY_DOWN },
        { GLFW_KEY_LEFT,  KeyName::KEY_LEFT },
        { GLFW_KEY_RIGHT, KeyName::KEY_RIGHT},

        { GLFW_KEY_SPACE,        KeyName::KEY_SPACE     },
        { GLFW_KEY_BACKSPACE,    KeyName::KEY_BACKSPACE },
        { GLFW_KEY_ENTER,        KeyName::KEY_ENTER     },
        { GLFW_KEY_LEFT_CONTROL, KeyName::KEY_LCTRL     },
        { GLFW_KEY_LEFT_SHIFT,   KeyName::KEY_SHIFT     },
        { GLFW_KEY_TAB,          KeyName::KEY_TAB       },
        { GLFW_KEY_ESCAPE,       KeyName::KEY_ESCAPE    }
    };

    static std::unordered_map<int, MouseButtonName> s_glfwToMouseButtonMapping
    {
        { GLFW_MOUSE_BUTTON_LEFT,   MouseButtonName::MOUSE_LEFT   },
        { GLFW_MOUSE_BUTTON_RIGHT,  MouseButtonName::MOUSE_RIGHT  },
        { GLFW_MOUSE_BUTTON_MIDDLE, MouseButtonName::MOUSE_MIDDLE }
    };

    static std::unordered_map<int, InputAction> s_glfwToActionMapping
    {
        { GLFW_RELEASE, InputAction::RELEASE },
        { GLFW_PRESS,   InputAction::PRESS }
    };


    static KeyName get_key_name(int glfwKey)
    {
        std::unordered_map<int, KeyName>::const_iterator it = s_glfwToKeyMapping.find(glfwKey);
        if (it != s_glfwToKeyMapping.end())
            return it->second;
        return KeyName::KEY_NOT_FOUND;
    }

    static MouseButtonName get_mouse_button_name(int glfwButton)
    {
        std::unordered_map<int, MouseButtonName>::const_iterator it = s_glfwToMouseButtonMapping.find(glfwButton);
        if (it != s_glfwToMouseButtonMapping.end())
            return it->second;
        return MouseButtonName::MOUSE_BUTTON_NOT_FOUND;
    }

    static InputAction get_input_action(int glfwAction)
    {
        std::unordered_map<int, InputAction>::const_iterator it = s_glfwToActionMapping.find(glfwAction);
        if (it != s_glfwToActionMapping.end())
            return it->second;
        return InputAction::ACTION_NOT_FOUND;
    }


    // NOTE: All below funcs are pretty new so there might be some keys/buttons wrong, etc..
    void key_callback(GLFWwindow* pGLFWwindow, int key, int scancode, int action, int mods)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        KeyName keyName = get_key_name(key);
        InputAction inputAction = get_input_action(action);
        pInputManager->_keyDown[keyName] = action != GLFW_RELEASE;
        pInputManager->processKeyEvents(keyName, scancode, inputAction, mods);
    }

    void char_callback(GLFWwindow* pGLFWwindow, unsigned int codepoint)
    {
        Debug::log("PROCESS CHARACTER EVENT: NOT TESTED!", Debug::MessageType::PLATYPUS_WARNING);
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        pInputManager->processCharInputEvents(codepoint);
    }

    void mouse_button_callback(GLFWwindow* pGLFWwindow, int button, int action, int mods)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        MouseButtonName buttonName = get_mouse_button_name(button);
        InputAction inputAction = get_input_action(action);
        pInputManager->_mouseDown[buttonName] = action == GLFW_PRESS;
        pInputManager->processMouseButtonEvents(buttonName, inputAction, mods);
    }

    void cursor_pos_callback(GLFWwindow* pGLFWwindow, double x, double y)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);

        // In web gl our coords are flipped because framebuffer coords
        // -> flip to have y increase downwards
        // TODO: Make this rather be swapchain extent?
        //const int windowHeight = Application::get_instance()->getWindow().getHeight();
        //int my = windowHeight - (int)y;

        int mx = (int)x;
        int my = (int)y;
        pInputManager->setMousePos(mx, my);
        pInputManager->processCursorPosEvents(mx, my);
    }

    void scroll_callback(GLFWwindow* pGLFWwindow, double xOffset, double yOffset)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        pInputManager->processScrollEvents(xOffset, yOffset);
    }

    static int s_lastFramebufferWidth = 0;
    static int s_lastFramebufferHeight = 0;
    void framebuffer_resize_callback(GLFWwindow* pGLFWwindow, int width, int height)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        pInputManager->processWindowResizeEvents(width, height);
        pInputManager->handleWindowResizing(width, height);
        s_lastFramebufferWidth = width;
        s_lastFramebufferHeight = height;
    }

    static void window_refresh_callback(GLFWwindow* pGLFWwindow)
    {
        InputManager* pInputManager = (InputManager*)glfwGetWindowUserPointer(pGLFWwindow);
        pInputManager->processWindowResizeEvents(s_lastFramebufferWidth, s_lastFramebufferHeight);
        pInputManager->handleWindowResizing(s_lastFramebufferWidth, s_lastFramebufferHeight);
    }

    InputManager::InputManager(Window& windowRef) :
        _windowRef(windowRef)
    {
        GLFWwindow* pGLFWwindow = windowRef.getImpl()->pGLFWwindow;
        glfwSetWindowUserPointer(pGLFWwindow, this);

        glfwSetKeyCallback(pGLFWwindow, key_callback);
        glfwSetCharCallback(pGLFWwindow, char_callback);
        glfwSetMouseButtonCallback(pGLFWwindow, mouse_button_callback);
        glfwSetCursorPosCallback(pGLFWwindow, cursor_pos_callback);
        glfwSetScrollCallback(pGLFWwindow, scroll_callback);
        glfwSetFramebufferSizeCallback(pGLFWwindow, framebuffer_resize_callback);
        glfwSetWindowRefreshCallback(pGLFWwindow, window_refresh_callback);
    }

    InputManager::~InputManager()
    {
        destroyEvents();
    }

    void InputManager::handleWindowResizing(int width, int height)
    {
        _windowRef._width = width;
        _windowRef._height = height;
        _windowRef._resized = true;
    }

    void InputManager::pollEvents()
    {
        glfwPollEvents();
    }

    void InputManager::waitEvents()
    {
        glfwWaitEvents();
    }
}
