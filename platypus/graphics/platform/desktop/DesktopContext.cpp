#include "platypus/graphics/Context.h"
#include "platypus/graphics/platform/desktop/DesktopContext.h"

#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <set>

#include "platypus/core/Debug.h"
#include "platypus/Common.h"
#include "platypus/core/platform/desktop/DesktopWindow.h"
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


    static std::vector<const char*> get_required_device_extensions()
    {
        const std::vector<const char*> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
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


    static bool check_device_extension_availability(
        VkPhysicalDevice physicalDevice,
        const std::vector<const char*>& extensions,
        std::vector<const char*>& outUnavailable
    )
    {
        uint32_t availableExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());
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


    static VkSurfaceKHR create_window_surface(VkInstance instance, Window* pWindow)
    {
        VkSurfaceKHR surface;
        VkResult result = glfwCreateWindowSurface(
            instance,
            pWindow->getImpl()->pGLFWwindow,
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
        return surface;
    }


    static ContextImpl::QueueFamilyIndices find_queue_families(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        ContextImpl::QueueFamilyIndices result;
        int index = 0;
        for (const VkQueueFamilyProperties& queueFamilyProperties : queueFamilies)
        {
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result.queueFlags |= ContextImpl::QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS;
                result.graphicsFamily = index;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
            if (presentSupport)
            {
                result.queueFlags |= ContextImpl::QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT;
                result.presentFamily = index;
            }
            ++index;
        }
        return result;
    }

    static ContextImpl::SwapchainSupportDetails query_swapchain_support_details(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        ContextImpl::SwapchainSupportDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.surfaceCapabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        details.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.surfaceFormats.data());

        uint32_t presentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, nullptr);
        details.presentModes.resize(presentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, details.presentModes.data());

        return details;
    };

    static bool is_device_adequate(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        // TODO:
        //  * Have some device limit requirements and check those here
        //  from VkPhysicalDeviceProperties
        //  * Some logging telling why device not adequate
        ContextImpl::QueueFamilyIndices queueFamilyIndices = find_queue_families(physicalDevice, surface);
        uint32_t requiredFlags = ContextImpl::QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS | ContextImpl::QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT;

        std::vector<const char*> requiredDeviceExtensions = get_required_device_extensions();
        std::vector<const char*> unavailableExtensions;
        bool supportsSwapchain = check_device_extension_availability(
            physicalDevice,
            requiredDeviceExtensions,
            unavailableExtensions
        );

        ContextImpl::SwapchainSupportDetails swapchainSupportDetails = query_swapchain_support_details(
            physicalDevice,
            surface
        );
        bool swapchainDetailsAdequate = !swapchainSupportDetails.surfaceFormats.empty() && !swapchainSupportDetails.presentModes.empty();

        return ((queueFamilyIndices.queueFlags & requiredFlags) == requiredFlags) && supportsSwapchain && swapchainDetailsAdequate;
    }

    static VkPhysicalDevice auto_pick_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        size_t& outMinUniformBufferOffsetAlignment
    )
    {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0)
        {
            Debug::log(
                "@pick_physical_device "
                "No physical devices found",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        Debug::log(
            "Found physical devices:"
        );
        for (const VkPhysicalDevice& physicalDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            // TODO:
            //  * Maybe display some more info?
            //  * Store device limits somewhere?
            std::string deviceNameStr(properties.deviceName);
            std::string deviceTypeStr = string_VkPhysicalDeviceType(properties.deviceType);
            Debug::log("    " + deviceNameStr);
            Debug::log("        type: " + deviceTypeStr);
            Debug::log("        api version: " + std::to_string(properties.apiVersion));

            if (is_device_adequate(physicalDevice, surface))
            {
                Debug::log("Picked device: " + deviceNameStr);
                outMinUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;
                return physicalDevice;
            }
        }
        Debug::log(
            "@auto_pick_physical_device "
            "Failed to find suitable physical device",
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return physicalDevices[0];
    }


    static VkDevice create_device(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        const std::vector<const char*>& extensions,
        const std::vector<const char*>& enabledLayers,
        VkQueue* pGraphicsQueue,
        VkQueue* pPresentQueue
    )
    {
        ContextImpl::QueueFamilyIndices queueFamilyIndices = find_queue_families(physicalDevice, surface);
        // It's possible that the chosen device has queue family supporting both graphics and
        // presentation so figure that out..
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures physicalDeviceFeatures{};

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

        deviceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

        deviceCreateInfo.enabledLayerCount = enabledLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();

        VkDevice device;
        VkResult createDeviceResult = vkCreateDevice(
            physicalDevice,
            &deviceCreateInfo,
            nullptr,
            &device
        );
        if (createDeviceResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createDeviceResult));
            Debug::log(
                "@create_device "
                "Failed to create VkDevice! VkResult: " + resultStr
            );
            PLATYPUS_ASSERT(false);
        }

        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily, 0, pGraphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily, 0, pPresentQueue);

        return device;
    }

    static VmaAllocator create_allocator(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkInstance instance
    )
    {
        VmaAllocatorCreateInfo createInfo{};
        createInfo.flags = 0;
        createInfo.physicalDevice = physicalDevice;
        createInfo.device = device;
        createInfo.instance = instance;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        //createInfo.pHeapSizeLimit = nullptr;
        VmaAllocator vmaAllocator;
        VkResult createResult = vmaCreateAllocator(&createInfo, &vmaAllocator);
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@create_allocator "
                "Failed to create VmaAllocator! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return vmaAllocator;
    }


    std::vector<VkImageView> create_image_views(
        VkDevice device,
        const std::vector<VkImage>& images,
        VkFormat format,
        VkImageAspectFlags aspectFlags
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
            createInfo.subresourceRange.levelCount = 1;
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


    Context* Context::s_pInstance = nullptr;
    Context::Context(const char* appName, Window* pWindow)
    {
        std::vector<const char*> requiredInstanceExtensions = get_required_instance_extensions();
        std::vector<const char*> requiredDeviceExtensions = get_required_device_extensions();
        std::vector<const char*> requiredLayers = get_required_layers();

        VkInstance instance = create_instance(
            appName,
            requiredInstanceExtensions,
            requiredLayers
        );

        #ifdef PLATYPUS_DEBUG
            VkDebugUtilsMessengerEXT debugMessenger = create_debug_messenger(instance);
        #endif

        VkSurfaceKHR surface = create_window_surface(instance, pWindow);

        VkPhysicalDevice selectedPhysicalDevice = auto_pick_physical_device(instance, surface, _minUniformBufferOffsetAlignment);
        ContextImpl::QueueFamilyIndices deviceQueueFamilyIndices = find_queue_families(selectedPhysicalDevice, surface);
        ContextImpl::SwapchainSupportDetails swapchainDetails = query_swapchain_support_details(selectedPhysicalDevice, surface);

        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkDevice device = create_device(
            selectedPhysicalDevice,
            surface,
            requiredDeviceExtensions,
            requiredLayers,
            &graphicsQueue,
            &presentQueue
        );

        VmaAllocator vmaAllocator = create_allocator(
            selectedPhysicalDevice,
            device,
            instance
        );

        _pImpl = new ContextImpl;
        _pImpl->instance = instance;
        #ifdef PLATYPUS_DEBUG
            _pImpl->debugMessenger = debugMessenger;
        #endif
        _pImpl->surface = surface;
        _pImpl->physicalDevice = selectedPhysicalDevice;
        _pImpl->device = device;
        _pImpl->vmaAllocator = vmaAllocator;
        _pImpl->deviceQueueFamilyIndices = deviceQueueFamilyIndices;
        _pImpl->graphicsQueue = graphicsQueue;
        _pImpl->presentQueue = presentQueue;
        _pImpl->deviceSwapchainSupportDetails = swapchainDetails;

        s_pInstance = this;

        Debug::log("Graphics Context created");
    }

    Context::~Context()
    {
        if (_pImpl->instance)
        {
            vmaDestroyAllocator(_pImpl->vmaAllocator);
            vkDestroyDevice(_pImpl->device, nullptr);
            vkDestroySurfaceKHR(_pImpl->instance, _pImpl->surface, nullptr);
            #ifdef PLATYPUS_DEBUG
                if (_pImpl->debugMessenger)
                    destroy_vk_debug_messenger(_pImpl->instance, _pImpl->debugMessenger, nullptr);
            #endif
            vkDestroyInstance(_pImpl->instance, nullptr);
        }
        delete _pImpl;
    }

    void Context::submitPrimaryCommandBuffer(Swapchain& swapchain, const CommandBuffer& cmdBuf, size_t frame)
    {
        SwapchainImpl* pSwapchainImpl = swapchain._pImpl;
        uint32_t imageIndex = swapchain.getCurrentImageIndex();
        // check if prev frame is using this image
        if (pSwapchainImpl->inFlightImages[imageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(_pImpl->device, 1, &pSwapchainImpl->inFlightImages[imageIndex], VK_TRUE, UINT64_MAX);

        // mark this img to be now used by this frame
        pSwapchainImpl->inFlightImages[imageIndex] = pSwapchainImpl->inFlightFences[frame];
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &pSwapchainImpl->imageAvailableSemaphores[frame];
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf._pImpl->handle;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &pSwapchainImpl->renderFinishedSemaphores[frame];

        vkResetFences(_pImpl->device, 1, &pSwapchainImpl->inFlightFences[frame]);
        VkResult submitResult = vkQueueSubmit(
            _pImpl->graphicsQueue,
            1,
            &submitInfo,
            pSwapchainImpl->inFlightFences[frame]
        );
        if (submitResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(submitResult));
            Debug::log(
                "@Context::submitPrimaryCommandBuffer "
                "Failed to submit command buffer to graphics queue! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void Context::waitForOperations()
    {
        vkDeviceWaitIdle(_pImpl->device);
    }

    void Context::handleWindowResize()
    {
        _pImpl->deviceSwapchainSupportDetails = query_swapchain_support_details(
            _pImpl->physicalDevice,
            _pImpl->surface
        );
    }

    Context* Context::get_instance()
    {
        if (!s_pInstance)
        {
            Debug::log(
                "@Context::get_instance "
                "Context instance was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return s_pInstance;
    }
}
