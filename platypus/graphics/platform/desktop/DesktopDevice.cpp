#include "platypus/graphics/Device.hpp"
#include "DesktopDevice.hpp"
#include "platypus/graphics/Context.hpp"
#include "DesktopContext.hpp"
#include "platypus/graphics/Swapchain.hpp"
#include "DesktopSwapchain.hpp"
#include "DesktopCommandBuffer.hpp"
#include "platypus/core/platform/desktop/DesktopWindow.hpp"
#include "platypus/assets/platform/desktop/DesktopTexture.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include <cstring>
#include <set>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    static std::set<VkFormat> s_allFormats = {
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R8G8B8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8_SRGB,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,

        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    static std::set<VkFormat> s_colorFormats = {
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R8G8B8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8_SRGB,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM
    };

    static std::set<VkFormat> s_depthFormats = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    // Returns which formats in possibleFormats are supported
    static std::unordered_map<VkFormat, VkFormatProperties> get_supported_formats(
        VkPhysicalDevice physicalDevice,
        const std::set<VkFormat>& possibleFormats
    )
    {
        std::unordered_map<VkFormat, VkFormatProperties> supportedFormats;
        for (VkFormat possibleFormat : possibleFormats)
        {
            VkFormatProperties possibleFormatProperties;
            vkGetPhysicalDeviceFormatProperties(
                physicalDevice,
                possibleFormat,
                &possibleFormatProperties
            );
            if (!(possibleFormatProperties.bufferFeatures == 0 &&
                possibleFormatProperties.linearTilingFeatures == 0 &&
                possibleFormatProperties.optimalTilingFeatures == 0)
            )
            {
                supportedFormats[possibleFormat] = possibleFormatProperties;
            }
        }
        return supportedFormats;
    }


    static std::vector<VkFormat> get_color_formats(const PhysicalDevice& physicalDevice)
    {
        std::vector<VkFormat> colorFormats;
        const std::unordered_map<VkFormat, VkFormatProperties>& supportedFormats = physicalDevice.supportedFormats;
        std::unordered_map<VkFormat, VkFormatProperties>::const_iterator it;
        for (it = supportedFormats.begin(); it != supportedFormats.end(); ++it)
        {
            if (s_colorFormats.find(it->first) != s_colorFormats.end())
                colorFormats.push_back(it->first);
        }
        return colorFormats;
    }


    static std::vector<VkFormat> get_depth_formats(const PhysicalDevice& physicalDevice)
    {
        std::vector<VkFormat> depthFormats;
        const std::unordered_map<VkFormat, VkFormatProperties>& supportedFormats = physicalDevice.supportedFormats;
        std::unordered_map<VkFormat, VkFormatProperties>::const_iterator it;
        for (it = supportedFormats.begin(); it != supportedFormats.end(); ++it)
        {
            if (s_depthFormats.find(it->first) != s_depthFormats.end())
                depthFormats.push_back(it->first);
        }
        return depthFormats;
    }


    static WindowSurfaceProperties get_window_surface_properties(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        WindowSurfaceProperties properties{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &properties.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        properties.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, properties.formats.data());

        uint32_t presentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, nullptr);
        properties.presentModes.resize(presentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, properties.presentModes.data());

        return properties;
    };


    static QueueProperties get_queue_properties(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        QueueProperties result;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        int index = 0;
        for (const VkQueueFamilyProperties& queueFamilyProperties : queueFamilies)
        {
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result.queueFlags |= QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS;
                result.graphicsFamilyIndex = index;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
            if (presentSupport)
            {
                result.queueFlags |= QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT;
                result.presentFamilyIndex = index;
            }
            ++index;
        }
        return result;
    }


    static std::vector<PhysicalDevice> get_physical_devices(
        VkInstance instance,
        VkSurfaceKHR windowSurface
    )
    {
        std::vector<PhysicalDevice> physicalDevices;

        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0)
            return physicalDevices;

        std::vector<VkPhysicalDevice> availableDevices(count);
        vkEnumeratePhysicalDevices(instance, &count, availableDevices.data());

        for (VkPhysicalDevice availableDevice : availableDevices)
        {
            PhysicalDevice physicalDevice{ availableDevice };
            vkGetPhysicalDeviceProperties(availableDevice, &physicalDevice.properties);

            physicalDevice.queueProperties = get_queue_properties(
                physicalDevice.handle,
                windowSurface
            );

            uint32_t deviceExtensionCount = 0;
            vkEnumerateDeviceExtensionProperties(
                availableDevice,
                nullptr,
                &deviceExtensionCount,
                nullptr
            );
            physicalDevice.extensionProperties.resize(deviceExtensionCount);
            vkEnumerateDeviceExtensionProperties(
                availableDevice,
                nullptr,
                &deviceExtensionCount,
                physicalDevice.extensionProperties.data()
            );

            physicalDevice.supportedFormats = get_supported_formats(availableDevice, s_allFormats);

            physicalDevice.windowSurfaceProperties = get_window_surface_properties(
                availableDevice,
                windowSurface
            );

            physicalDevices.push_back(physicalDevice);
        }

        return physicalDevices;
    }


    // Returns error messages for each missing feature.
    // Returns empty vec if device is adequate.
    static std::vector<std::string> is_device_adequate(
        const PhysicalDevice& physicalDevice,
        VkSurfaceKHR windowSurface,
        const std::vector<std::string>& requiredExtensions
    )
    {
        std::vector<std::string> errors;
        if (!(physicalDevice.queueProperties.queueFlags & QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS))
            errors.push_back("No graphics queue found");
        if (!(physicalDevice.queueProperties.queueFlags & QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT))
            errors.push_back("No present queue found");

        std::vector<std::string> missingExtensions;
        for (const std::string& requiredExtension : requiredExtensions)
        {
            bool found = false;
            for (VkExtensionProperties availableExtension : physicalDevice.extensionProperties)
            {
                if (strcmp(requiredExtension.c_str(), availableExtension.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                missingExtensions.push_back(requiredExtension);
        }
        if (!missingExtensions.empty())
        {
            for (const std::string& missingExtension : missingExtensions)
                errors.push_back("Missing extension: " + missingExtension);
        }

        // TODO: Make sure that required formats are supported!
        if (physicalDevice.supportedFormats.empty())
            errors.push_back("No supported formats found");

        if (physicalDevice.windowSurfaceProperties.formats.empty())
            errors.push_back("No window surface formats found");
        if (physicalDevice.windowSurfaceProperties.presentModes.empty())
            errors.push_back("No window surface present modes found");

        return errors;
    }


    DeviceImpl* Device::s_pImpl = nullptr;
    Window* Device::s_pWindow = nullptr;
    size_t Device::s_minUniformBufferOffsetAlignment = 0;
    CommandPool* Device::s_pCommandPool = nullptr;
    std::vector<ImageFormat> Device::s_supportedDepthFormats;
    std::vector<ImageFormat> Device::s_supportedColorFormats;

    void Device::create(Window* pWindow)
    {
        s_pWindow = pWindow;

        ContextImpl* pContextImpl = Context::get_impl();
        if (!pContextImpl)
        {
            Debug::log(
                "ContextImpl was nullptr! "
                "Make sure you have created context before creating the Device!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        VkInstance instance = pContextImpl->instance;
        if (instance == VK_NULL_HANDLE)
        {
            Debug::log(
                "VkInstance was VK_NULL_HANDLE "
                "Make sure you have created context before creating the Device!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        VkSurfaceKHR surface = pWindow->getImpl()->surface;
        if (surface == VK_NULL_HANDLE)
        {
            Debug::log(
                "Window's VkSurfaceKHR was VK_NULL_HANDLE "
                "Make sure you have created the window before creating the Device!",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::vector<std::string> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        std::vector<PhysicalDevice> availableDevices = get_physical_devices(instance, surface);
        std::vector<PhysicalDevice> adequateDevices;

        Debug::log("Found devices:");
        for (const PhysicalDevice& availableDevice : availableDevices)
        {
            Debug::log("    " + std::string(availableDevice.properties.deviceName));
            std::vector<std::string> missingFeatures = is_device_adequate(
                availableDevice,
                surface,
                requiredExtensions
            );
            if (missingFeatures.empty())
            {
                adequateDevices.push_back(availableDevice);
            }
            else
            {
                std::string msg = "        ->Invalid for application's usage due to following issues:\n";
                for (size_t i = 0; i < missingFeatures.size(); ++i)
                {
                    msg += "            " + missingFeatures[i];
                    if (i != missingFeatures.size() - 1)
                        msg += "\n";
                }
                Debug::log(msg);
            }
        }

        if (adequateDevices.empty())
        {
            Debug::log(
                "No adequate devices found!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Debug::log("Usable devices:");
        for (const PhysicalDevice& adequateDevice : adequateDevices)
            Debug::log("    " + std::string(adequateDevice.properties.deviceName));

        // Selecting first adequate device for now JUST FOR TESTING!
        PhysicalDevice selectedPhysicalDevice = adequateDevices[0];
        s_minUniformBufferOffsetAlignment = selectedPhysicalDevice.properties.limits.minUniformBufferOffsetAlignment;
        QueueProperties queueProperties = selectedPhysicalDevice.queueProperties;

        // It's possible that the chosen device has queue family supporting both graphics and
        // presentation so figure that out..
        std::set<uint32_t> uniqueQueueFamilyIndices = {
            queueProperties.graphicsFamilyIndex,
            queueProperties.presentFamilyIndex
        };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t queueFamilyIndex : uniqueQueueFamilyIndices)
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
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

        std::vector<const char*> useExtensions(requiredExtensions.size());
        for (size_t i = 0; i < requiredExtensions.size(); ++i)
            useExtensions[i] = requiredExtensions[i].c_str();

        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(useExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = useExtensions.data();

        VkDevice device;
        VkResult createDeviceResult = vkCreateDevice(
            selectedPhysicalDevice.handle,
            &deviceCreateInfo,
            nullptr,
            &device
        );
        if (createDeviceResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createDeviceResult));
            Debug::log(
                "Failed to create VkDevice! VkResult: " + resultStr,
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        // NOTE: Should we care about the queue indices here (set to 0)
        // in addition to the family indices?
        vkGetDeviceQueue(device, queueProperties.graphicsFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueProperties.presentFamilyIndex, 0, &presentQueue);

        s_pImpl = new DeviceImpl;
        s_pImpl->physicalDevice = selectedPhysicalDevice;
        s_pImpl->device = device;
        s_pImpl->graphicsQueue = graphicsQueue;
        s_pImpl->presentQueue = presentQueue;

        // Fucking dumb, but will do for now...
        for (VkFormat colorFormat : get_color_formats(selectedPhysicalDevice))
            s_supportedColorFormats.push_back(to_engine_format(colorFormat));

        for (VkFormat depthFormat : get_depth_formats(selectedPhysicalDevice))
            s_supportedDepthFormats.push_back(to_engine_format(depthFormat));

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = 0;
        allocatorCreateInfo.physicalDevice = selectedPhysicalDevice.handle;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = Context::get_impl()->instance;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        //allocatorCreateInfo.pHeapSizeLimit = nullptr;
        VmaAllocator vmaAllocator;
        VkResult createResult = vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Device::create "
                "Failed to create VmaAllocator! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        s_pImpl->vmaAllocator = vmaAllocator;

        s_pCommandPool = new CommandPool;
    }

    void Device::destroy()
    {
        s_supportedColorFormats.clear();
        s_supportedDepthFormats.clear();

        if (!s_pImpl)
        {
            Debug::log(
                "@Device::destroy "
                "s_pImpl was nullptr! Make sure device was created successfully before destroying it.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        delete s_pCommandPool;
        vmaDestroyAllocator(s_pImpl->vmaAllocator);
        vkDestroyDevice(s_pImpl->device, nullptr);

        s_pImpl->physicalDevice = { };
    }

    void Device::submit_primary_command_buffer(
        Swapchain& swapchain,
        const CommandBuffer& cmdBuf,
        size_t frame
    )
    {
        SwapchainImpl* pSwapchainImpl = swapchain.getImpl();

        // NOTE: This could be its own Swapchain member function since we're waiting for the
        // swapchain here...
        uint32_t imageIndex = swapchain.getCurrentImageIndex();
        // check if prev frame is using this image
        if (pSwapchainImpl->inFlightImages[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(
                s_pImpl->device,
                1,
                &pSwapchainImpl->inFlightImages[imageIndex],
                VK_TRUE,
                UINT64_MAX
            );
        }

        // mark this img to be now used by this frame
        pSwapchainImpl->inFlightImages[imageIndex] = pSwapchainImpl->inFlightFences[frame];
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &pSwapchainImpl->imageAvailableSemaphores[frame];
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf.getImpl()->handle;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &pSwapchainImpl->renderFinishedSemaphores[imageIndex];

        vkResetFences(s_pImpl->device, 1, &pSwapchainImpl->inFlightFences[frame]);
        VkResult submitResult = vkQueueSubmit(
            s_pImpl->graphicsQueue,
            1,
            &submitInfo,
            pSwapchainImpl->inFlightFences[frame]
        );
        if (submitResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(submitResult));
            Debug::log(
                "@Device::submitPrimaryCommandBuffer "
                "Failed to submit command buffer to graphics queue! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void Device::wait_for_operations()
    {
        vkDeviceWaitIdle(s_pImpl->device);
    }

    void Device::handle_window_resize()
    {
        s_pImpl->physicalDevice.windowSurfaceProperties = get_window_surface_properties(
            s_pImpl->physicalDevice.handle,
            s_pWindow->getImpl()->surface
        );
    }

    size_t Device::get_min_uniform_buffer_offset_align()
    {
        return s_minUniformBufferOffsetAlignment;
    }

    CommandPool* Device::get_command_pool()
    {
        return s_pCommandPool;
    }

    DeviceImpl* Device::get_impl()
    {
        #ifdef PLATYPUS_DEBUG
        if (!s_pImpl)
        {
            Debug::log(
                "@Device::get_impl "
                "Device's s_pImpl was nullptr! "
                "Make sure you have created the device throught Application "
                "or explicitly with Device::create.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        #endif
        return s_pImpl;
    }

    bool is_format_supported(VkFormat format)
    {
        const PhysicalDevice& physicalDevice = Device::get_impl()->physicalDevice;
        if (physicalDevice.handle == VK_NULL_HANDLE)
        {
            Debug::log(
                "Physical device was VK_NULL_HANDLE",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return physicalDevice.supportedFormats.find(format) != physicalDevice.supportedFormats.end();
    }
}
