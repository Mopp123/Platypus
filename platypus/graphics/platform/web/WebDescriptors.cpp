#include "platypus/graphics/Descriptors.h"

namespace platypus
{
    DescriptorSetLayout::DescriptorSetLayout(
        const std::vector<DescriptorSetLayoutBinding>& bindings
    )
    {
    }

    struct DescriptorSetLayoutImpl
    {
    };

    DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayout& other)
    {
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
    }

    void DescriptorSetLayout::destroy()
    {
    }


    struct DescriptorSetImpl
    {
    };

    DescriptorSet::DescriptorSet(const std::vector<const Buffer*>& buffers)
    {
    }

    DescriptorSet::DescriptorSet(const std::vector<const Texture*>& textures)
    {
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other)
    {
    }

    DescriptorSet::DescriptorSet& operator=(DescriptorSet&& other)
    {
    }

    DescriptorSet::~DescriptorSet()
    {
    }
}
