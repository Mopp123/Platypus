#pragma once

#include "Buffers.h"
#include "Swapchain.h"
#include "platypus/assets/Texture.h"
#include <memory>


namespace platypus
{
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_NONE = 0x0,
        DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 0x1,
        DESCRIPTOR_TYPE_UNIFORM_BUFFER = 0x2,
        DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER = 0x3
    };


    /*  Specifies Uniform buffer struct's layout
     *  NOTE: This is only used by web platform (webgl1/ES2)
     *  since no actual uniform buffers available
     *
     *  Example:
     *
     *  struct DirectionalLight
     *  {
     *      vec3 direction;
     *      vec3 color;
     *  };
     *
     *  above should specify UniformBufferInfo's layout as:
     *  {
     *      ShaderDataType::Float3, ShaderDataType::Float3
     *  }
     *
     *  location index means how many"th" uniform in shader's all uniforms this one is at
     * */
    struct UniformInfo
    {
        ShaderDataType type;
        // NOTE: should be using DescriptorSetLayoutBinding's "descriptorCount" instead of "arrayLen"
        // when I was writing this initially, I forgot how Vulkan deals with that kind of stuff...
        int arrayLen = 1;

        bool operator==(const UniformInfo& other) const
        {
            return type == other.type && arrayLen == other.arrayLen;
        }
    };


    class DescriptorSetLayoutBinding
    {
    private:
        uint32_t _binding = 0;
        DescriptorType _type = DescriptorType::DESCRIPTOR_TYPE_NONE;
        uint32_t _shaderStageFlags = 0;
        uint32_t _descriptorCount = 0;

        std::vector<UniformInfo> _uniformInfo;

    public:

        // NOTE: ADDED RECENTLY! NOT SURE IF BREAKS SOMETHING?!?!
        DescriptorSetLayoutBinding() {}
        // NOTE:
        //  * If using this binding as an array, descriptorCount is the length of the array
        //  * Don't remember why I allowed having multiple shader stage flags...
        DescriptorSetLayoutBinding(
            uint32_t binding,
            uint32_t descriptorCount,
            DescriptorType type,
            unsigned int shaderStageFlags,
            std::vector<UniformInfo> uniformInfo
        ) :
            _binding(binding),
            _type(type),
            _shaderStageFlags(shaderStageFlags),
            _descriptorCount(descriptorCount),
            _uniformInfo(uniformInfo)
        {}

        DescriptorSetLayoutBinding(const DescriptorSetLayoutBinding& other) :
            _binding(other._binding),
            _type(other._type),
            _shaderStageFlags(other._shaderStageFlags),
            _descriptorCount(other._descriptorCount),
            _uniformInfo(other._uniformInfo)
        {}

        ~DescriptorSetLayoutBinding() {}

        bool operator==(const DescriptorSetLayoutBinding& other) const
        {
            return _binding == other._binding &&
                _type == other._type &&
                _shaderStageFlags == other._shaderStageFlags &&
                _descriptorCount == other._descriptorCount &&
                _uniformInfo == other._uniformInfo;
        }

        inline uint32_t getBinding() const { return _binding; }
        inline DescriptorType getType() const { return _type; }
        inline uint32_t getShaderStageFlags() const { return _shaderStageFlags; }
        inline uint32_t getDescriptorCount() const { return _descriptorCount; }
        inline const std::vector<UniformInfo>& getUniformInfo() const { return _uniformInfo; }
    };


    struct DescriptorSetLayoutImpl;
    class DescriptorSetLayout
    {
    private:
        DescriptorSetLayoutImpl* _pImpl = nullptr;
        std::vector<DescriptorSetLayoutBinding> _bindings;

    public:
        DescriptorSetLayout();
        DescriptorSetLayout(const std::vector<DescriptorSetLayoutBinding>& bindings);
        DescriptorSetLayout(const DescriptorSetLayout& other);
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other);
        //DescriptorSetLayout& operator=(DescriptorSetLayout& other);
        ~DescriptorSetLayout();

        void destroy();

        bool operator==(const DescriptorSetLayout& other) const { return _bindings == other._bindings; }

        inline const std::vector<DescriptorSetLayoutBinding>& getBindings() const { return _bindings; }
        inline const DescriptorSetLayoutImpl* getImpl() const { return _pImpl; }
    };


    struct DescriptorSetComponent
    {
        DescriptorType type = DescriptorType::DESCRIPTOR_TYPE_NONE;
        const void* pData = nullptr;
        bool depthImageTEST = false;
    };


    class DescriptorPool;

    struct DescriptorSetImpl;
    // NOTE: Descriptor sets ARE ment to be copyable and assignable.
    // -> They're just describing!
    class DescriptorSet
    {
    private:
        friend class DescriptorPool;
        std::shared_ptr<DescriptorSetImpl> _pImpl = nullptr;

    public:
        DescriptorSet();

        // NOTE: There has been quite a lot of weird issues when copying descriptor sets!
        // PREVIOUS ISSUE: The way descriptor sets are created using DescriptorPool, had to add
        // assignment operator that fixed an issue where segfaulting due to original descriptor set going
        // out of scope, that hadn't set its members correctly...
        DescriptorSet(const DescriptorSet& other);
        DescriptorSet& operator=(DescriptorSet other);
        // Don't remember why I ever had below
        //      -> copy constructor gets implicitly deleted if below is defined!
        //          -> caused some issues, but this might cause issues in the future as well?
        //DescriptorSet& operator=(DescriptorSet&& other);

        ~DescriptorSet();

        void update(
            DescriptorPool& descriptorPool,
            uint32_t binding,
            DescriptorSetComponent component
        );

        inline const DescriptorSetImpl* getImpl() const { return _pImpl.get(); }
        inline DescriptorSetImpl* getImpl() { return _pImpl.get(); }
    };


    struct DescriptorPoolImpl;
    class DescriptorPool
    {
    private:
        DescriptorPoolImpl* _pImpl = nullptr;

    public:
        DescriptorPool(const Swapchain& swapchain);
        ~DescriptorPool();

        DescriptorSet createDescriptorSet(
            const DescriptorSetLayout& layout,
            const std::vector<DescriptorSetComponent>& components
        );

        void freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets);
        inline DescriptorPoolImpl* getImpl() { return _pImpl; }
    };
}
