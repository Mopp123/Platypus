#include "platypus/graphics/Context.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>

#include "platypus/core/Debug.h"
#include "platypus/Common.h"


namespace platypus
{
    struct ContextImpl
    {
        VkInstance instance;
    };

    Context::Context(const char* appName)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = PLATYPUS_ENGINE_NAME;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        uint32_t glfwRequiredExtensionCount = 0;
        const char** glfwRequiredExtensions;
        glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

        instanceCreateInfo.enabledExtensionCount = glfwRequiredExtensionCount;
        instanceCreateInfo.ppEnabledExtensionNames = glfwRequiredExtensions;

        // TODO: Enable validation layer for dev build
        instanceCreateInfo.enabledLayerCount = 0;

        VkInstance instance;
        VkResult createInstanceResult = vkCreateInstance(
            &instanceCreateInfo,
            nullptr,
            &instance
        );
        if (createInstanceResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createInstanceResult));
            Debug::log(
                "@Context::Context "
                "Failed to create VkInstance! VkResult: " + resultStr
            );
            PLATYPUS_ASSERT(false);
        }

        // NOTE: Not sure sh
        _pImpl = new ContextImpl;
        _pImpl->instance = instance;
    }

    Context::~Context()
    {
        if (_pImpl->instance)
            vkDestroyInstance(_pImpl->instance, nullptr);
        delete _pImpl;
    }
}
