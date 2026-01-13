#include "platypus/graphics/Descriptors.h"
#include "WebDescriptors.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    static void update_descriptor_pool_set(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        size_t descriptorSetID,
        uint32_t binding,
        DescriptorSetComponent component
    )
    {
        std::unordered_map<ID_t, std::vector<DescriptorSetComponent>>& poolSetData = pDescriptorPoolImpl->descriptorSetData;
        std::unordered_map<ID_t, std::vector<DescriptorSetComponent>>::iterator it = poolSetData.find(descriptorSetID);
        if (it == poolSetData.end())
        {
            Debug::log(
                "@update_descriptor_pool_set "
                "Failed to find descriptor set with id " + std::to_string(descriptorSetID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::vector<DescriptorSetComponent>& setComponents = it->second;
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


    // TODO: Make safer?
    DescriptorSetComponent* get_pool_descriptor_set_data(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID,
        uint32_t binding
    )
    {
        return &pDescriptorPoolImpl->descriptorSetData[descriptorSetID][(size_t)binding];
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
        _pImpl = std::make_shared<DescriptorSetImpl>();
    }

    DescriptorSet::DescriptorSet(const DescriptorSet& other)
    {
        _pImpl = other._pImpl;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet other)
    {
        _pImpl = other._pImpl;
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
        update_descriptor_pool_set(
            descriptorPool.getImpl(),
            _pImpl->id,
            binding,
            component
        );
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
        DescriptorSet newDescriptorSet;
        ID_t id = ID::generate();
        newDescriptorSet._pImpl->id = id;
        _pImpl->descriptorSetData[id] = components;
        return newDescriptorSet;
    }

    // NOTE: No idea if this works properly!
    void DescriptorPool::freeDescriptorSets(const std::vector<DescriptorSet>& descriptorSets)
    {
        std::unordered_map<ID_t, std::vector<DescriptorSetComponent>>& poolSetData = _pImpl->descriptorSetData;
        for (const DescriptorSet& descriptorSet : descriptorSets)
        {
            std::unordered_map<ID_t, std::vector<DescriptorSetComponent>>::iterator it = poolSetData.find(descriptorSet._pImpl->id);
            if (it == poolSetData.end())
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
            poolSetData.erase(it);
        }
    }
}
