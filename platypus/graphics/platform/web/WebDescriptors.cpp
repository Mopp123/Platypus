#include "platypus/graphics/Descriptors.h"

namespace platypus
{
    struct DescriptorSetLayoutImpl
    {
    };

    DescriptorSetLayout::DescriptorSetLayout(
        const std::vector<DescriptorSetLayoutBinding>& bindings
    ) :
        _bindings(bindings)
    {
    }

    DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayout& other) :
        _bindings(other._bindings)
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

    DescriptorSet::DescriptorSet(const std::vector<const Buffer*>& buffers, const DescriptorSetLayout* pLayout) :
        _buffers(buffers),
        _pLayout(pLayout)
    {
    }

    DescriptorSet::DescriptorSet(const std::vector<const Texture*>& textures, const DescriptorSetLayout* pLayout) :
        _textures(textures),
        _pLayout(pLayout)
    {
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _buffers(other._buffers),
        _textures(other._textures),
        _pLayout(other._pLayout)
    {
    }

    DescriptorSet::DescriptorSet& operator=(DescriptorSet&& other)
    {
        _buffers = other._buffers;
        _textures = other._textures;
        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {
    }


    DescriptorPool::DescriptorPool(const Swapchain& swapchain)
    {
    }

    DescriptorPool::~DescriptorPool()
    {
    }


    struct DescriptorPoolImpl
    {
    };

    // Buffer and/or Texture has to be provided for each binding in the layout!
    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<const Buffer*>& buffers
    )
    {
        // NOTE: Not sure if works properly, not tested yet!
        return { buffers, pLayout };
    }

    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<const Texture*>& textures
    )
    {
        // NOTE: Not sure if works properly, not tested yet!
        return { textures, pLayout };
    }
}
