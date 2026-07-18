#pragma once

#include <platypus/graphics/Descriptors.hpp>
#include <platypus/utils/UUID.hpp>
#include <unordered_map>


namespace platypus
{
    struct DescriptorSetImpl
    {
        UUID_t id = NULL_UUID;
    };

    struct DescriptorPoolImpl
    {
        size_t uuidPool = NULL_UUID;
        std::unordered_map<UUID_t, std::vector<DescriptorSetComponent>> descriptorSetData;

        DescriptorPoolImpl();
        ~DescriptorPoolImpl();
    };

    // TODO: Make safer?
    DescriptorSetComponent* get_pool_descriptor_set_data(
        DescriptorPoolImpl* pDescriptorPoolImpl,
        UUID_t descriptorSetID,
        uint32_t binding
    );
}
