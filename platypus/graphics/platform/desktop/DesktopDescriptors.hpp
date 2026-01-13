#pragma once

#include "platypus/graphics/Descriptors.hpp"
#include <vulkan/vulkan.h>


#define PLATY_MAX_DESCRIPTOR_SETS 1000


namespace platypus
{
    struct DescriptorSetLayoutImpl
    {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;
    };

    struct DescriptorSetImpl
    {
        VkDescriptorSet handle = VK_NULL_HANDLE;
    };

    struct DescriptorPoolImpl
    {
        VkDescriptorPool handle = VK_NULL_HANDLE;
    };

    VkDescriptorType to_vk_descriptor_type(const DescriptorType& type);
}
