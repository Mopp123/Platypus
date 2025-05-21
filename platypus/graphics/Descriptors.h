#pragma once

#include "Buffers.h"
#include "Swapchain.h"
#include "platypus/assets/Texture.h"


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
        int locationIndex;
        ShaderDataType type;
        // NOTE: should be using DescriptorSetLayoutBinding's "descriptorCount" instead of "arrayLen"
        // when I was writing this initially, I forgot how Vulkan deals with that kind of stuff...
        int arrayLen = 1;
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
        DescriptorSetLayout& operator=(DescriptorSetLayout& other) = delete;
        ~DescriptorSetLayout();

        void destroy();

        inline const std::vector<DescriptorSetLayoutBinding>& getBindings() const { return _bindings; }
        inline const DescriptorSetLayoutImpl* getImpl() const { return _pImpl; }
    };


    struct DescriptorSetComponent
    {
        DescriptorType type = DescriptorType::DESCRIPTOR_TYPE_NONE;
        const void* pData = nullptr;
    };


    class DescriptorPool;

    struct DescriptorSetImpl;
    class DescriptorSet
    {
    private:
        friend class DescriptorPool;
        DescriptorSetImpl* _pImpl = nullptr;
        std::vector<DescriptorSetComponent> _components;

        const DescriptorSetLayout* _pLayout;

    public:
        DescriptorSet() = default;

        DescriptorSet(
            const std::vector<DescriptorSetComponent>& components,
            const DescriptorSetLayout* pLayout
        );

        DescriptorSet(const DescriptorSet& other);
        DescriptorSet& operator=(DescriptorSet&& other);

        ~DescriptorSet();

        inline const std::vector<DescriptorSetComponent>& getComponents() const { return _components; }
        inline const DescriptorSetLayout* getLayout() const { return _pLayout; }
        inline const DescriptorSetImpl* getImpl() const { return _pImpl; }
    };


    struct DescriptorPoolImpl;
    class DescriptorPool
    {
    private:
        DescriptorPoolImpl* _pImpl = nullptr;

    public:
        DescriptorPool(const Swapchain& swapchain);
        ~DescriptorPool();

        // Buffer and/or Texture has to be provided for each binding in the layout!
        /*
        DescriptorSet createDescriptorSet(
            const DescriptorSetLayout* pLayout,
            const std::vector<const Buffer*>& buffers
        );
        DescriptorSet createDescriptorSet(
            const DescriptorSetLayout* pLayout,
            const std::vector<const Texture*>& textures
        );
        */


        DescriptorSet createDescriptorSet(
            const DescriptorSetLayout* pLayout,
            const std::vector<DescriptorSetComponent>& components
        );

        void freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets);
    };
}
