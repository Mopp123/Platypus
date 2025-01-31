#include "DesktopDescriptors.h"
#include "platypus/core/Debug.h"
#include "DesktopShader.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    VkDescriptorType to_vk_descriptor_type(const DescriptorType& type)
    {
        switch (type)
        {
            case DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            default:
                Debug::log(
                    "@to_vk_descriptor_type "
                    "Invalid DescriptorType: " + std::to_string(type) + " "
                    "Available types are: "
                    "DESCRIPTOR_TYPE_UNIFORM_BUFFER, "
                    "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
        }
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }


    DescriptorSetLayout::DescriptorSetLayout(const std::vector<DescriptorSetLayoutBinding>& bindings) :
        _bindings(bindings)
    {
        // NOTE: Not sure is zero initialization quaranteed here always?
        std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings(bindings.size());
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            const DescriptorSetLayoutBinding& binding = bindings[i];
            vkLayoutBindings[i].binding = binding.getBinding();
            vkLayoutBindings[i].descriptorCount = binding.getDescriptorCount();
            vkLayoutBindings[i].descriptorType = to_vk_descriptor_type(binding.getType());
            vkLayoutBindings[i].stageFlags = to_vk_shader_stage_flags(binding.getShaderStageFlags());
            vkLayoutBindings[i].pImmutableSamplers = nullptr; // This is used with image sampling related descriptors
        }
        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = (uint32_t)vkLayoutBindings.size();
        createInfo.pBindings = vkLayoutBindings.data();

        VkDescriptorSetLayout handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateDescriptorSetLayout(
            Context::get_impl()->device,
            &createInfo,
            nullptr,
            &handle
        );

        if (createResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createResult));
            Debug::log(
                "@DescriptorSetLayout::DescriptorSetLayout "
                "Failed to create descriptor set layout! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new DescriptorSetLayoutImpl;
        _pImpl->handle = handle;
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        if(_pImpl)
        {
            vkDestroyDescriptorSetLayout(
                Context::get_impl()->device,
                _pImpl->handle,
                nullptr
            );
            delete _pImpl;
        }
    }


    DescriptorSet::DescriptorSet(
        const DescriptorSetLayout& layout,
        std::vector<const Buffer*> pBuffers
    ) :
        _layoutRef(layout),
        _pBuffers(pBuffers)
    {}

    DescriptorSet::~DescriptorSet()
    {
        if (_pImpl)
            delete _pImpl;
    }
}
