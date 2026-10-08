#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "Component.hpp"
#include "platypus/core/Memory.hpp"
#include <cstdint>
#include <vector>


namespace platypus
{
    enum class CustomDataType : uint32_t
    {
        INT,
        UINT,
        FLOAT,
        STRING,
        VECTOR2F,
        VECTOR3F,
        VECTOR4F
    };

    size_t get_custom_data_type_size(CustomDataType type);
    std::string custom_data_type_to_string(CustomDataType type);
    std::vector<CustomDataType> get_available_custom_data_types();

    constexpr size_t serialized_custom_data_base_size =
        sizeof(ComponentType) +
        sizeof(uint32_t); // element count

    struct CustomDataValue
    {
        CustomDataType type;
        uint32_t usedDataSize = 0;
        uint32_t maxDataSize = 0;
        const void* pData = nullptr;
    };

    struct SerializedCustomDataValue
    {
        CustomDataType type;
        uint32_t dataSize = 0;
        std::vector<uint8_t> data;
    };

    struct CustomData
    {
        int32_t offset = -1;
        uint32_t elementCount = 0;
    };

    class Scene;
    CustomData* create_custom_data(
        entityID_t target,
        const std::vector<SerializedCustomDataValue>& values = { },
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    size_t get_serialized_custom_data_value_size(const CustomDataValue * const pCustomDataValue);
    size_t get_serialized_custom_data_size(const CustomData * const pCustomData);
    size_t get_serialized_custom_data_size(const char* pSerializedData, size_t dataSize);
    std::vector<char> serialize(const CustomDataValue * const pCustomDataValue);
    std::vector<char> serialize(const CustomData * const pCustomData);

    void deserialize(
        Scene* pScene,
        CustomData** ppCustomData,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );

    class CustomDataManager
    {
    private:
        // Mem layout:
        //  CustomDataType type
        //  data[size of type]
        struct StoredCustomDataValue
        {
            CustomDataType type;
            std::vector<uint8_t> data;
        };

        // Mem layout:
        //  uint32_t elemCount
        //  values[elemCount]
        DynamicElementSizeMemoryPool _memoryPool;

    public:
        int32_t add(
            CustomData* pCustomData,
            CustomDataType type,
            size_t dataSize,
            const void* pData
        );
        int32_t update(int32_t offset, const std::string& newStr);
        void remove(int32_t offset);

    private:
        StoredCustomDataValue toStoredCustomDataValue(
            int32_t offset
        ) const;

        std::vector<StoredCustomDataValue> copyValues(
            int32_t offset
        ) const;

        bool validateOffset(int32_t offset) const;

        inline size_t getTotalStoredValueSize(const StoredCustomDataValue& value) const { return sizeof(CustomDataType) + value.data.size(); }
    };
}
