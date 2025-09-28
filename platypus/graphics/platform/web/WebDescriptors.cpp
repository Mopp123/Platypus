#include "platypus/graphics/Descriptors.h"

namespace platypus
{
    struct DescriptorSetLayoutImpl
    {
    };

    DescriptorSetLayout::DescriptorSetLayout()
    {
    }

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

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other)
    {
        _bindings = other._bindings;
        return *this;
    }

    //DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout& other)
    //{
    //    _bindings = other._bindings;
    //    return *this;
    //}

    DescriptorSetLayout::~DescriptorSetLayout()
    {
    }

    void DescriptorSetLayout::destroy()
    {
    }


    struct DescriptorSetImpl
    {
    };

    DescriptorSet::DescriptorSet()
    {
    }

    DescriptorSet::DescriptorSet(
        const std::vector<DescriptorSetComponent>& components,
        const DescriptorSetLayout* pLayout
    ) :
        _components(components),
        _pLayout(pLayout)
    {
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _components(other._components),
        _pLayout(other._pLayout)
    {
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet other)
    {
        _components = other._components;
        _pLayout = other._pLayout;
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

    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout* pLayout,
        const std::vector<DescriptorSetComponent>& components
    )
    {
        // NOTE: Not sure if works properly, not tested yet!
        return { components, pLayout };
    }

    void DescriptorPool::freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets)
    {
    }
}
