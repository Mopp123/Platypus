#pragma once

#include <platypus/graphics/Descriptors.h>
#include <platypus/utils/ID.h>
#include <unordered_map>


namespace platypus
{
    struct DescriptorSetImpl
    {
        ID_t id = NULL_ID;
    };

    struct DescriptorPoolImpl
    {
        std::unordered_map<ID_t, DescriptorSet> descriptorSets;
    };

    DescriptorSet get_pool_descriptor_set(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID
    );

    DescriptorSetComponent* get_pool_descriptor_set_data(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        ID_t descriptorSetID,
        uint32_t binding
    );
}
