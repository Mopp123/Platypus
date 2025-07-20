#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


namespace platypus
{
    struct WindowImpl
    {
        GLFWwindow* pGLFWwindow = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
    };
}
