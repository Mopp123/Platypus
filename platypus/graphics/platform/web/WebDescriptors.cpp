#include "platypus/graphics/Descriptors.h"
#include "WebDescriptors.hpp"
#include "platypus/core/Debug.h"


namespace platypus
{
    static void update_descriptor_pool_set(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        size_t descriptorSetID,
        uint32_t binding,
        DescriptorSetComponent component
    )
    {
        std::unordered_map<ID_t, DescriptorSet>& poolSets = pDescriptorPoolImpl->descriptorSets;
        std::unordered_map<ID_t, DescriptorSet>::iterator it = poolSets.find(descriptorSetID);
        if (it == poolSets.end())
        {
            Debug::log(
                "@update_descriptor_pool_set "
                "Failed to find descriptor set with id " + std::to_string(descriptorSetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        DescriptorSet& descriptorSet = pDescriptorPoolImpl->descriptorSets[descriptorSetID];
        std::vector<DescriptorSetComponent>& setComponents = descriptorSet.getComponents();
        if (binding >= setComponents.size())
        {
            Debug::log(
                "@update_descriptor_pool_set "
                "binding " + std::to_string(binding) + " out of bounds! "
                "Descriptor set (id: " + std::to_string(descriptorSetID) + ") "
                "has " + std::to_string(setComponents.size()) + " components",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        setComponents[(size_t)binding] = component;
    }


    DescriptorSet get_pool_descriptor_set(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID
    )
    {
        std::unordered_map<ID_t, DescriptorSet>& poolSets = pDescriptorPoolImpl->descriptorSets;
        std::unordered_map<ID_t, DescriptorSet>::iterator it = poolSets.find(descriptorSetID);
        if (it == poolSets.end())
        {
            Debug::log(
                "@get_pool_descriptor_set "
                "Failed to find descriptor set with id " + std::to_string(descriptorSetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return {};
        }
        return it->second;
    }

    DescriptorSetComponent* get_pool_descriptor_set_data(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID,
        uint32_t binding
    )
    {
        return &pDescriptorPoolImpl->descriptorSets[descriptorSetID].getComponents()[(size_t)binding];
    }

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


    DescriptorSet::DescriptorSet()
    {
    }

    DescriptorSet::DescriptorSet(
        const std::vector<DescriptorSetComponent>& components
    ) :
        _components(components)
    {
        _pImpl = std::make_shared<DescriptorSetImpl>();
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other) :
        _components(other._components)
    {
        _pImpl = other._pImpl;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet other)
    {
        _pImpl = other._pImpl;
        _components = other._components;
        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {
    }

    void DescriptorSet::update(
        DescriptorPool& descriptorPool,
        uint32_t binding,
        DescriptorSetComponent component
    )
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

        update_descriptor_pool_set(
            descriptorPool.getImpl(),
            _pImpl->id,
            binding,
            component
        );

        _components[binding] = component;
    }

    DescriptorPool::DescriptorPool(const Swapchain& swapchain)
    {
        _pImpl = new DescriptorPoolImpl;
    }

    DescriptorPool::~DescriptorPool()
    {
        if (_pImpl)
        {
            delete _pImpl;
        }
    }


    DescriptorSet DescriptorPool::createDescriptorSet(
        const DescriptorSetLayout& layout,
        const std::vector<DescriptorSetComponent>& components
    )
    {
        DescriptorSet newDescriptorSet(components);
        ID_t id = ID::generate();
        newDescriptorSet._pImpl->id = id;
        _pImpl->descriptorSets[id] = newDescriptorSet;
        return newDescriptorSet;
    }

    // NOTE: No idea if this works properly!
    void DescriptorPool::freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets)
    {
        std::unordered_map<ID_t, DescriptorSet>& poolSets = _pImpl->descriptorSets;
        for (const DescriptorSet& descriptorSet : descriptorSets)
        {
            std::unordered_map<ID_t, DescriptorSet>::iterator it = poolSets.find(descriptorSet._pImpl->id);
            if (it == poolSets.end())
            {
                Debug::log(
                    "@DescriptorPool::freeDescriptorSets "
                    "Failed to find descriptor set with id " + std::to_string(descriptorSet._pImpl->id),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            ID::erase(descriptorSet._pImpl->id);
            poolSets.erase(it);
        }
    }
}
