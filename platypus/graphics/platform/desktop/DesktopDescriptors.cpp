#include "DesktopDescriptors.h"
#include "platypus/core/Debug.h"
#include "DesktopShader.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "DesktopSwapchain.h"
#include "DesktopBuffers.h"
#include "platypus/assets/platform/desktop/DesktopTexture.h"
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    VkDescriptorType to_vk_descriptor_type(const DescriptorType& type)
    {
        switch (type)
        {
            case DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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


    DescriptorSetLayout::DescriptorSetLayout()
    {
        _pImpl = new DescriptorSetLayoutImpl;
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
            Context::get_instance()->getImpl()->device,
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

    DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayout& other) :
        _bindings(other._bindings)
    {
        _pImpl = new DescriptorSetLayoutImpl;
        _pImpl->handle = other._pImpl->handle;
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        // NOTE: Shouldn't call destroy here since this may have been copied to another
        // layout which still uses the vk handle
        if(_pImpl)
            delete _pImpl;
    }

    void DescriptorSetLayout::destroy()
    {
        vkDestroyDescriptorSetLayout(
            Context::get_instance()->getImpl()->device,
            _pImpl->handle,
            nullptr
        );
        _pImpl->handle = VK_NULL_HANDLE;
    }


    DescriptorSet::DescriptorSet(
        const std::vector<DescriptorSetComponent>& components,
        const DescriptorSetLayout* pLayout
    ) :
        _components(components),
        _pLayout(pLayout)
    {
        _pImpl = new DescriptorSetImpl;
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _components(other._components),
        _pLayout(other._pLayout)
    {
        _pImpl = new DescriptorSetImpl;
        _pImpl->handle = other._pImpl->handle;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other)
    {
        _components = other._components;
        _pImpl = new DescriptorSetImpl;
        _pImpl->handle = other._pImpl->handle;
        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {
        if (_pImpl)
            delete _pImpl;
    }


    static VkDescriptorSet alloc_descriptor_set(
        VkDescriptorPool vkDescriptorPool,
        const DescriptorSetLayout* pLayout
    )
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vkDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &pLayout->getImpl()->handle;

        VkDescriptorSet descriptorSetHandle;
        VkDevice device = Context::get_instance()->getImpl()->device;
        VkResult allocResult = vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &descriptorSetHandle
        );
        if (allocResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(allocResult));
            Debug::log(
                "@alloc_descriptor_set "
                "Failed to allocate descriptor set! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return descriptorSetHandle;
    }


    DescriptorPool::DescriptorPool(const Swapchain& swapchain)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;

        const size_t& maxFramesInFlight = swapchain.getImpl()->maxFramesInFlight;
        // NOTE: maxDescriptorSets hardcoded here ONLY FOR TESTING!
        const uint32_t maxDescriptorSets = PLATY_MAX_DESCRIPTOR_SETS * maxFramesInFlight;
        const std::vector<DescriptorType> types =
        {
            DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
            DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        };

        for (size_t i = 0; i < types.size(); ++i)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = to_vk_descriptor_type(types[i]);
            poolSize.descriptorCount = maxDescriptorSets; //maxFramesInFlight;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = (uint32_t)poolSizes.size();
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = maxDescriptorSets;
        createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkDescriptorPool handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateDescriptorPool(
            Context::get_instance()->getImpl()->device,
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
                Context::get_instance()->getImpl()->device,
                _pImpl->handle,
                nullptr
            );
            delete _pImpl;
        }
    }

    // Buffer and/or Texture has to be provided for each binding in the layout!
    /*
    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<const Buffer*>& buffers
    )
    {
        const std::vector<DescriptorSetLayoutBinding>& bindings = pLayout->getBindings();
        if (bindings.size() != buffers.size())
        {
            Debug::log(
                "@DescriptorPool::createDescriptorSets(1) "
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
        VkDevice device = Context::get_instance()->getImpl()->device;
        VkResult allocResult = vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &descriptorSetHandle
        );
        if (allocResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(allocResult));
            Debug::log(
                "@DescriptorPool::createDescriptorSets(1) "
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
            // NOTE: range was pBuffer->getTotalSize() earlier because didn't use "dynamic uniform buffers"
            // -> meaning large buffer used with dynamic offsets instead of whole buffer when binding
            // descriptor sets! -> And range being the element size should make sence for all cases
            //  -> YOU JUST NEED TO BE CAREFUL THAT YOU PROVIDE THE CORRECT ELEMENT SIZE!
            bufferInfo.range = pBuffer->getDataElemSize();
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

        DescriptorSet createdDescriptorSet(buffers, pLayout);
        createdDescriptorSet._pImpl->handle = descriptorSetHandle;
        return createdDescriptorSet;
    }


    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<const Texture*>& textures
    )
    {
        const std::vector<DescriptorSetLayoutBinding>& bindings = pLayout->getBindings();
        if (bindings.size() != textures.size())
        {
            Debug::log(
                "@DescriptorPool::createDescriptorSets(2) "
                "Texture required for each layout's binding! "
                "Provided bindings: " + std::to_string(bindings.size()) + " "
                "textures: " + std::to_string(textures.size()),
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
        VkDevice device = Context::get_instance()->getImpl()->device;
        VkResult allocResult = vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &descriptorSetHandle
        );
        if (allocResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(allocResult));
            Debug::log(
                "@DescriptorPool::createDescriptorSets(2) "
                "Failed to allocate descriptor set! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        for (size_t i = 0; i < bindings.size(); ++i)
        {
            const Texture* pTexture = textures[i];
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = pTexture->getImpl()->imageView;
            imageInfo.sampler = pTexture->getSamplerImpl()->handle;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.dstSet = descriptorSetHandle;
            descriptorWrite.dstBinding = (uint32_t)i;
            descriptorWrite.dstArrayElement = 0; // if using array descriptors, this is the first index in the array..
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            descriptorWrite.pImageInfo = &imageInfo;
            descriptorWrite.pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }

        DescriptorSet createdDescriptorSet(textures, pLayout);
        createdDescriptorSet._pImpl->handle = descriptorSetHandle;
        return createdDescriptorSet;
    }
    */


    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<DescriptorSetComponent>& components
    )
    {
        const std::vector<DescriptorSetLayoutBinding>& bindings = pLayout->getBindings();
        if (bindings.size() != components.size())
        {
            Debug::log(
                "@DescriptorPool::createDescriptorSet "
                "Component required for each layout's binding! "
                "Provided bindings: " + std::to_string(bindings.size()) + " "
                "components: " + std::to_string(components.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        VkDescriptorSet vkDescriptorSetHandle = alloc_descriptor_set(_pImpl->handle, pLayout);

        for (size_t i = 0; i < bindings.size(); ++i)
        {
            const DescriptorSetLayoutBinding& binding = bindings[i];

            VkDescriptorBufferInfo bufferInfo{};
            VkDescriptorImageInfo imageInfo{};

            DescriptorType bindingType = binding.getType();
            if (components[i].type != bindingType)
            {
                Debug::log(
                    "@DescriptorPool::createDescriptorSet "
                    "Invalid descriptor set component type: " + std::to_string(components[i].type) + " " +
                    "for binding at index: " + std::to_string(i) + " " +
                    "binding is using type: " + std::to_string(bindingType),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            bool isTexture = false;
            if (bindingType == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER || bindingType == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
            {
                // NOTE: Danger?
                const Buffer* pBuffer = (const Buffer*)components[i].pData;
                bufferInfo.buffer = pBuffer->getImpl()->handle;
                bufferInfo.offset = 0;
                // NOTE: range was pBuffer->getTotalSize() earlier because didn't use "dynamic uniform buffers"
                // -> meaning large buffer used with dynamic offsets instead of whole buffer when binding
                // descriptor sets! -> And range being the element size should make sence for all cases
                //  -> YOU JUST NEED TO BE CAREFUL THAT YOU PROVIDE THE CORRECT ELEMENT SIZE!
                bufferInfo.range = pBuffer->getDataElemSize();
            }
            else if (bindingType == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                isTexture = true;
                // NOTE: Danger?
                const Texture* pTexture = (const Texture*)components[i].pData;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = pTexture->getImpl()->imageView;
                imageInfo.sampler = pTexture->getSamplerImpl()->handle;
            }
            else
            {
                Debug::log(
                    "@DescriptorPool::createDescriptorSet "
                    "Invalid type for descriptor set layout binding at index: " + std::to_string(i),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            // NOTE: No idea is this going to work...
            // need to write to multiple bindings in some cases...
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.dstSet = vkDescriptorSetHandle;
            descriptorWrite.dstBinding = binding.getBinding();
            descriptorWrite.dstArrayElement = 0; // if using array descriptors, this is the first index in the array..
            descriptorWrite.descriptorType = to_vk_descriptor_type(binding.getType());

            if (!isTexture)
                descriptorWrite.pBufferInfo = &bufferInfo;
            else
                descriptorWrite.pImageInfo = &imageInfo;

            descriptorWrite.pTexelBufferView = nullptr; // what this?

            vkUpdateDescriptorSets(
                Context::get_instance()->getImpl()->device,
                1,
                &descriptorWrite,
                0,
                nullptr
            );
        }

        DescriptorSet createdDescriptorSet(components, pLayout);
        createdDescriptorSet._pImpl->handle = vkDescriptorSetHandle;
        return createdDescriptorSet;
    }


    void DescriptorPool::freeDescriptorSets(
        const std::vector<DescriptorSet>& descriptorSets
    )
    {
        if (!_pImpl)
        {
            Debug::log(
                "@DescriptorPool::freeDescriptorSets "
                "Descriptor pool pimpl was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        for (const DescriptorSet& descriptorSet : descriptorSets)
        {
            const DescriptorSetImpl* pDescriptorSetImpl = descriptorSet.getImpl();
            if (!pDescriptorSetImpl)
            {
                Debug::log(
                    "@DescriptorPool::freeDescriptorSets "
                    "Descriptor set's impl was nullptr!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            vkFreeDescriptorSets(
                Context::get_instance()->getImpl()->device,
                _pImpl->handle,
                1,
                &pDescriptorSetImpl->handle
            );
        }
    }
}
