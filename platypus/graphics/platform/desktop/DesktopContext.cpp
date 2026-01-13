#include "platypus/graphics/Context.hpp"
#include "platypus/graphics/platform/desktop/DesktopContext.hpp"

#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <set>

#include "platypus/core/Debug.hpp"
#include "platypus/core/platform/desktop/DesktopWindow.hpp"

#include "platypus/Common.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/platform/desktop/DesktopSwapchain.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/platform/desktop/DesktopCommandBuffer.h"


namespace platypus
{
    static std::vector<const char*> get_required_instance_extensions()
    {
        std::vector<const char*> requiredExtensions;
        uint32_t glfwRequiredExtensionCount = 0;
        const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
        for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
            requiredExtensions.push_back(glfwRequiredExtensions[i]);

        #ifdef PLATYPUS_DEBUG
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif
        return requiredExtensions;
    }


    static std::vector<const char*> get_required_layers()
    {
        std::vector<const char*> requiredLayers;
        #ifdef PLATYPUS_DEBUG
            requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
        #endif
        return requiredLayers;
    }


    static bool check_instance_extension_availability(
        const std::vector<const char*>& extensions,
        std::vector<const char*>& outUnavailable
    )
    {
        uint32_t availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());
        for (size_t i = 0; i < extensions.size(); ++i)
        {
            bool found = false;
            for (const VkExtensionProperties& availableExtension : availableExtensions)
            {
                if (strcmp(extensions[i], availableExtension.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                outUnavailable.push_back(extensions[i]);
        }
        return outUnavailable.empty();
    }


    static bool check_layer_availability(const std::vector<const char*>& layers, std::vector<const char*>& outUnavailable)
    {
        uint32_t availableLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(availableLayerCount);
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

        for (size_t i = 0; i < layers.size(); ++i)
        {
            bool found = false;
            for (const VkLayerProperties& availableLayer : availableLayers)
            {
                if (strcmp(layers[i], availableLayer.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                outUnavailable.push_back(layers[i]);
        }
        return outUnavailable.empty();
    }


    static VkInstance create_instance(
        const char* appName,
        const std::vector<const char*>& extensions,
        const std::vector<const char*>& layers
    )
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = PLATYPUS_ENGINE_NAME;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 1, 0);

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        // Check extensions
        std::vector<const char*> unavailableExtensions;
        if (!check_instance_extension_availability(extensions, unavailableExtensions))
        {
            std::string unavailableListStr;
            for (size_t i = 0; i < unavailableExtensions.size(); ++i)
                unavailableListStr += std::string(unavailableExtensions[i]) + (i == unavailableExtensions.size() - 1 ? "" : ", ");
            Debug::log(
                "@Context::Context "
                "Failed to create context due to unavailable extensions: " + unavailableListStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        instanceCreateInfo.enabledExtensionCount = extensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        // Check layers
        std::vector<const char*> unavailableLayers;
        if (!check_layer_availability(layers, unavailableLayers))
        {
            std::string unavailableListStr;
            for (size_t i = 0; i < unavailableLayers.size(); ++i)
                unavailableListStr += std::string(unavailableLayers[i]) + (i == unavailableLayers.size() - 1 ? "" : ", ");
            Debug::log(
                "@Context::Context "
                "Failed to create context due to unavailable layers: " + unavailableListStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        instanceCreateInfo.enabledLayerCount = layers.size();
        instanceCreateInfo.ppEnabledLayerNames = layers.data();

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
                "@create_instance "
                "Failed to create VkInstance! VkResult: " + resultStr
            );
            PLATYPUS_ASSERT(false);
        }
        return instance;
    }


    #ifdef PLATYPUS_DEBUG
        VkResult create_vk_debug_messenger(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger
        )
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            else
                return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        void destroy_vk_debug_messenger(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator
        )
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
                func(instance, debugMessenger, pAllocator);
        }

        static Debug::MessageType vk_debug_msg_severity_to_engine(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
        {
            switch (severity)
            {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return Debug::MessageType::PLATYPUS_MESSAGE;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return Debug::MessageType::PLATYPUS_MESSAGE;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return Debug::MessageType::PLATYPUS_WARNING;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return Debug::MessageType::PLATYPUS_ERROR;
                default: return Debug::MessageType::PLATYPUS_MESSAGE;
            }
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_message_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
        )
        {
            // TODO: Some build flags to specify what severities and types to log!
            if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                Debug::MessageType engineMsgType = vk_debug_msg_severity_to_engine(severity);
                Debug::log(
                    "@Vulkan Validation Layer: " + std::string(pCallbackData->pMessage),
                    engineMsgType
                );
                if (engineMsgType == Debug::MessageType::PLATYPUS_ERROR)
                {
                    // Apparently there was something (probably?) wrong when recreating swapchain
                    // due to validation layer getting surface capabilities' extent few pixels wrong from
                    // the one we get from glfwGetFramebufferSize(...)
                    if (strcmp(pCallbackData->pMessageIdName, "VUID-VkSwapchainCreateInfoKHR-imageExtent-01274") != 0)
                        PLATYPUS_ASSERT(false);
                    else
                        Debug::log(
                            "@Vulkan Validation Layer: NOTE! "
                            "Error assert was skipped due to swapchain extent validation issue!",
                            Debug::MessageType::PLATYPUS_WARNING
                        );
                }
            }
            return VK_FALSE;
        }

        static VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance instance)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
            debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugMessengerCreateInfo.pfnUserCallback = debug_message_callback;

            VkDebugUtilsMessengerEXT debugMessenger;
            VkResult createDebugMessengerResult = create_vk_debug_messenger(
                instance,
                &debugMessengerCreateInfo,
                nullptr,
                &debugMessenger
            );
            if (createDebugMessengerResult != VK_SUCCESS)
            {
                Debug::log(
                    "@create_debug_messenger "
                    "Failed to create VkDebugUtilsMessengerEXT which is required for debug build!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            return debugMessenger;
        }
    #endif


    static void create_window_surface(VkInstance instance, Window* pWindow)
    {
        WindowImpl* pWindowImpl = pWindow->getImpl();
        VkSurfaceKHR surface;
        VkResult result = glfwCreateWindowSurface(
            instance,
            pWindowImpl->pGLFWwindow,
            nullptr,
            &surface
        );
        if (result != VK_SUCCESS)
        {
            std::string resultStr(string_VkResult(result));
            Debug::log(
                "@create_window_surface "
                "glfwCreateWindowSurface failed! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        pWindowImpl->surface = surface;
    }


    std::vector<VkImageView> create_image_views(
        VkDevice device,
        const std::vector<VkImage>& images,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipLevelCount
    )
    {
        std::vector<VkImageView> imageViews(images.size());
        for (size_t i = 0; i < images.size(); ++i)
        {
            const VkImage& image = images[i];
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = aspectFlags;
            createInfo.subresourceRange.baseMipLevel = 0; // TODO: Mipmapping!
            createInfo.subresourceRange.levelCount = mipLevelCount;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult createResult = vkCreateImageView(
                device,
                &createInfo,
                nullptr,
                &imageViews[i]
            );
            if (createResult != VK_SUCCESS)
            {
                const std::string errStr(string_VkResult(createResult));
                Debug::log(
                    "@create_image_views "
                    "Failed to create image view at index: " + std::to_string(i) + " "
                    "using aspect flag: " + std::to_string(aspectFlags) + " "
                    "VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        return imageViews;
    }


    Window* Context::s_pWindow = nullptr;
    ContextImpl* Context::s_pImpl = nullptr;
    void Context::create(const char* appName, Window* pWindow)
    {
        s_pWindow = pWindow;

        std::vector<const char*> requiredInstanceExtensions = get_required_instance_extensions();
        std::vector<const char*> requiredLayers = get_required_layers();

        VkInstance instance = create_instance(
            appName,
            requiredInstanceExtensions,
            requiredLayers
        );

        #ifdef PLATYPUS_DEBUG
            VkDebugUtilsMessengerEXT debugMessenger = create_debug_messenger(instance);
        #endif

        create_window_surface(instance, pWindow);

        s_pImpl = new ContextImpl;
        s_pImpl->instance = instance;
        #ifdef PLATYPUS_DEBUG
            s_pImpl->debugMessenger = debugMessenger;
        #endif

        Debug::log("Graphics Context created");
    }

    void Context::destroy()
    {
        if (s_pImpl->instance)
        {
            VkInstance instance = s_pImpl->instance;
            vkDestroySurfaceKHR(
                instance,
                s_pWindow->getImpl()->surface,
                nullptr
            );
            s_pWindow->getImpl()->surface = VK_NULL_HANDLE;
            #ifdef PLATYPUS_DEBUG
                if (s_pImpl->debugMessenger)
                {
                    destroy_vk_debug_messenger(
                        s_pImpl->instance,
                        s_pImpl->debugMessenger,
                        nullptr
                    );
                }
            #endif
            vkDestroyInstance(s_pImpl->instance, nullptr);
        }
        delete s_pImpl;
    }

    ContextImpl* Context::get_impl()
    {
        return s_pImpl;
    }
}
