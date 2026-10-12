#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "Component.hpp"
#include "platypus/core/Memory.hpp"
#include "platypus/core/StringPool.hpp"
#include <cstdint>
#include <vector>


namespace platypus
{
    enum class CustomDataType : uint32_t
    {
        NONE = 0,
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
        CustomDataType type = CustomDataType::NONE;
        uint32_t dataSize = 0;
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
        struct StoredCustomDataValue
        {
            CustomDataType type = CustomDataType::NONE;
            std::vector<uint8_t> data;
        };

        StringPool& _stringPoolRef;

        // Mem layout:
        //  uint32_t size (the whole size for all the used elements + the uint32_t that holds that size)
        //  uint8_t valuesBuffer[size]
        DynamicElementSizeMemoryPool _memoryPool;

    public:
        CustomDataManager(StringPool& stringPool);
        int32_t addNumber(
            CustomData* pCustomData,
            CustomDataType type,
            const void* pData
        );

        int32_t addString(
            CustomData* pCustomData,
            const std::string& str
        );

        // TODO: Make safer?
        // *More informative error if accidentally attempt to update value of wrong type/wrong index?
        void updateNumber(
            int32_t offset,
            size_t valueIndex,
            CustomDataType valueType,
            const void* pValueData
        );

        void updateString(
            int32_t offset,
            size_t valueIndex,
            const std::string& str
        );

        void remove(CustomData* pCustomData, size_t valueIndex);
        std::vector<CustomDataValue> getValues(int32_t offset) const;

        template<typename T>
        T getNumericValue(int32_t offset, size_t valueIndex) const;
        std::string getStringValue(const CustomData * const pCustomData, size_t valueIndex) const;

        std::string valueToString(const CustomDataValue& value) const;

        inline size_t getStorageSize() const { return _memoryPool.getTotalSize(); }

    private:
        const void* getValueData(
            size_t baseOffset,
            size_t valueSize,
            size_t valueIndex
        ) const;

        std::vector<StoredCustomDataValue> copyValues(
            int32_t offset
        ) const;

        bool validateOffset(int32_t offset) const;

        static void free_storage_func(size_t offset, size_t size, void* pUserData);
        static bool validate_free_range_func(size_t offset, size_t size, void* pUserData);

        inline size_t getTotalStoredValueSize(const StoredCustomDataValue& value) const { return sizeof(CustomDataType) + value.data.size(); }
    };
}
