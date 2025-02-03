#include "DesktopDescriptors.h"
#include "platypus/core/Debug.h"
#include "DesktopShader.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "DesktopSwapchain.h"
#include "DesktopBuffers.h"
#include <vulkan/vk_enum_string_helper.h>


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
        const DescriptorSetLayout* pLayout,
        const std::vector<const Buffer*>& pBuffers
    ) :
        _pLayout(pLayout),
        _pBuffers(pBuffers)
    {
        _pImpl = new DescriptorSetImpl;
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _pLayout(other._pLayout),
        _pBuffers(other._pBuffers)
    {
        _pImpl = new DescriptorSetImpl;
        _pImpl->handle = other._pImpl->handle;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other)
    {
        _pLayout = other._pLayout;
        _pBuffers = other._pBuffers;
        _pImpl = new DescriptorSetImpl;
        _pImpl->handle = other._pImpl->handle;
        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {
        if (_pImpl)
            delete _pImpl;
    }


    DescriptorPool::DescriptorPool(const Swapchain& swapchain)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;

        const size_t& maxFramesInFlight = swapchain.getImpl()->maxFramesInFlight;
        const uint32_t maxDescriptorSets = 1 * maxFramesInFlight;
        const std::vector<DescriptorType> types =
        {
            DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER
        };

        for (size_t i = 0; i < types.size(); ++i)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = to_vk_descriptor_type(types[i]);
            poolSize.descriptorCount = maxFramesInFlight;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = (uint32_t)poolSizes.size();
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = maxDescriptorSets;

        VkDescriptorPool handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateDescriptorPool(
            Context::get_impl()->device,
            &createInfo,
            nullptr,
            &handle
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createResult));
            Debug::log(
                "@DescriptorPool::DescriptorPool "
                "Failed to create descriptor pool! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _pImpl = new DescriptorPoolImpl;
        _pImpl->handle = handle;

        Debug::log("Descriptor pool created");
    }

    DescriptorPool::~DescriptorPool()
    {
        if (_pImpl)
        {
            vkDestroyDescriptorPool(
                Context::get_impl()->device,
                _pImpl->handle,
                nullptr
            );
            delete _pImpl;
        }
    }

    // Buffer has to be provided for each binding in the layout!
    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<const Buffer*>& buffers
    )
    {
        const std::vector<DescriptorSetLayoutBinding>& bindings = pLayout->getBindings();
        if (bindings.size() != buffers.size())
        {
            Debug::log(
                "@DescriptorPool::createDescriptorSets "
                "Buffer required for each layout's binding! "
                "Provided bindings: " + std::to_string(bindings.size()) + " "
                "buffers: " + std::to_string(buffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _pImpl->handle;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &pLayout->getImpl()->handle;

        VkDescriptorSet descriptorSetHandle;
        VkDevice device = Context::get_impl()->device;
        VkResult allocResult = vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &descriptorSetHandle
        );
        if (allocResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(allocResult));
            Debug::log(
                "@DescriptorPool::createDescriptorSets "
                "Failed to allocate descriptor set! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            const Buffer* pBuffer = buffers[i];
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = pBuffer->getImpl()->handle;
            bufferInfo.offset = 0;
            bufferInfo.range = pBuffer->getTotalSize();
            bufferInfos.push_back(bufferInfo);

            const DescriptorSetLayoutBinding& binding = bindings[i];
            // NOTE: No idea is this going to work...
            // need to write to multiple bindings in some cases...
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.dstSet = descriptorSetHandle;
            descriptorWrite.dstBinding = binding.getBinding();
            descriptorWrite.dstArrayElement = 0; // if using array descriptors, this is the first index in the array..
            descriptorWrite.descriptorType = to_vk_descriptor_type(binding.getType());
            descriptorWrite.pBufferInfo = &bufferInfo;

            descriptorWrite.pImageInfo = nullptr; // if image sampling descriptor
            descriptorWrite.pTexelBufferView = nullptr; // if image sampling descriptor

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }

        DescriptorSet createdDescriptorSet(pLayout, buffers);
        createdDescriptorSet._pImpl->handle = descriptorSetHandle;
        return createdDescriptorSet;
    }
}
