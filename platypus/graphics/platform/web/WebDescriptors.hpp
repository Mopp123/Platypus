#pragma once

#include <platypus/graphics/Descriptors.hpp>
#include <platypus/utils/ID.hpp>
#include <unordered_map>


namespace platypus
{
    struct DescriptorSetImpl
    {
        ID_t id = NULL_ID;
    };

    struct DescriptorPoolImpl
    {
        std::unordered_map<ID_t, std::vector<DescriptorSetComponent>> descriptorSetData;
    };

    // TODO: Make safer?
    DescriptorSetComponent* get_pool_descriptor_set_data(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID,
        uint32_t binding
    );
}
