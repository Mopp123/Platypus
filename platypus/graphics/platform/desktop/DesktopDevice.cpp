#include "platypus/graphics/Device.hpp"
#include "DesktopDevice.hpp"
#include "platypus/graphics/Context.hpp"
#include "DesktopContext.hpp"
#include "platypus/graphics/Swapchain.h"
#include "DesktopSwapchain.h"
#include "DesktopCommandBuffer.h"
#include "platypus/core/platform/desktop/DesktopWindow.hpp"
#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>
#include <cstring>
#include <set>
#include <vulkan/vulkan_core.h>


namespace platypus
{
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


    static DeviceImpl::QueueFamilyIndices find_queue_families(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        DeviceImpl::QueueFamilyIndices result;
        int index = 0;
        for (const VkQueueFamilyProperties& queueFamilyProperties : queueFamilies)
        {
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                result.queueFlags |= DeviceImpl::QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS;
                result.graphicsFamily = index;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
            if (presentSupport)
            {
                result.queueFlags |= DeviceImpl::QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT;
                result.presentFamily = index;
            }
            ++index;
        }
        return result;
    }


    static DeviceImpl::SurfaceDetails query_surface_details(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    )
    {
        DeviceImpl::SurfaceDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());

        uint32_t presentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, nullptr);
        details.presentModes.resize(presentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, details.presentModes.data());

        return details;
    };


    static bool is_device_adequate(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        const std::vector<const char*>& requiredExtensions
    )
    {
        // TODO:
        //  * Have some device limit requirements and check those here
        //  from VkPhysicalDeviceProperties
        //  * Some logging telling why device not adequate
        DeviceImpl::QueueFamilyIndices queueFamilyIndices = find_queue_families(physicalDevice, surface);
        uint32_t requiredFlags = DeviceImpl::QueueFamilyFlagBits::QUEUE_FAMILY_GRAPHICS | DeviceImpl::QueueFamilyFlagBits::QUEUE_FAMILY_PRESENT;

        std::vector<const char*> unavailableExtensions;
        bool supportsSwapchain = check_device_extension_availability(
            physicalDevice,
            requiredExtensions,
            unavailableExtensions
        );

        DeviceImpl::SurfaceDetails surfaceDetails = query_surface_details(
            physicalDevice,
            surface
        );
        bool swapchainDetailsAdequate = !surfaceDetails.formats.empty() && !surfaceDetails.presentModes.empty();

        return ((queueFamilyIndices.queueFlags & requiredFlags) == requiredFlags) && supportsSwapchain && swapchainDetailsAdequate;
    }


    static VkPhysicalDevice auto_pick_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char*>& requiredExtensions,
        size_t& outMinUniformBufferOffsetAlignment
    )
    {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0)
        {
            Debug::log(
                "@auto_pick_physical_device "
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

            if (is_device_adequate(physicalDevice, surface, requiredExtensions) && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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


    DeviceImpl* Device::s_pImpl = nullptr;
    Window* Device::s_pWindow = nullptr;
    size_t Device::s_minUniformBufferOffsetAlignment = 1;
    CommandPool* Device::s_pCommandPool = nullptr;
    void Device::create(Window* pWindow)
    {
        s_pWindow = pWindow;

        VkSurfaceKHR surface = pWindow->getImpl()->surface;

        std::vector<const char*> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkPhysicalDevice selectedPhysicalDevice = auto_pick_physical_device(
            Context::get_impl()->instance,
            surface,
            requiredExtensions,
            s_minUniformBufferOffsetAlignment
        );
        DeviceImpl::QueueFamilyIndices queueFamilyIndices = find_queue_families(
            selectedPhysicalDevice,
            surface
        );
        DeviceImpl::SurfaceDetails surfaceDetails = query_surface_details(
            selectedPhysicalDevice,
            surface
        );

        // It's possible that the chosen device has queue family supporting both graphics and
        // presentation so figure that out..
        std::set<uint32_t> uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamily,
            queueFamilyIndices.presentFamily
        };

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

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &physicalDeviceFeatures;

        createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        VkDevice device;
        VkResult createDeviceResult = vkCreateDevice(
            selectedPhysicalDevice,
            &createInfo,
            nullptr,
            &device
        );
        if (createDeviceResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createDeviceResult));
            Debug::log(
                "@Device::create "
                "Failed to create VkDevice! VkResult: " + resultStr
            );
            PLATYPUS_ASSERT(false);
        }

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily, 0, &presentQueue);

        s_pImpl = new DeviceImpl;
        s_pImpl->physicalDevice = selectedPhysicalDevice;
        s_pImpl->device = device;
        s_pImpl->queueFamilyIndices = queueFamilyIndices;
        s_pImpl->graphicsQueue = graphicsQueue;
        s_pImpl->presentQueue = presentQueue;
        s_pImpl->surfaceDetails = surfaceDetails;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = 0;
        allocatorCreateInfo.physicalDevice = selectedPhysicalDevice;
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
        s_pImpl->surfaceDetails = query_surface_details(
            s_pImpl->physicalDevice,
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
        return s_pImpl;
    }
}
