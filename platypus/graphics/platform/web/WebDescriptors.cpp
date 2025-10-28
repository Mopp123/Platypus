#include "platypus/graphics/Descriptors.h"
#include "platypus/core/Debug.h"


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
        const std::vector<DescriptorSetComponent>& components
    ) :
        _components(components)
    {
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _components(other._components)
    {
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet other)
    {
        _components = other._components;
        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {
    }

    void DescriptorSet::update(uint32_t binding, DescriptorSetComponent component)
    {
        if (binding >= _components.size())
        {
            Debug::log(
                "@DescriptorSet::update "
                "Binding(" + std::to_string(binding) + ") out of bounds. "
                "This DescriptorSet has " + std::to_string(_components.size()) + " components",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        _components[binding] = component;
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
        const DescriptorSetLayout& layout,
        const std::vector<DescriptorSetComponent>& components
    )
    {
        // NOTE: Not sure if works properly, not tested yet!
        return { components };
    }

    void DescriptorPool::freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets)
    {
    }
}
