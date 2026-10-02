#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "Component.hpp"
#include <cstdint>
#include <vector>
#include <map>


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
        const size_t _valueBaseSize = sizeof(uint32_t) * 3;

        // _data layout:
        //  uint32_t elementCount
        //  values[elementCount]
        std::vector<uint8_t> _data;

        std::map<size_t, size_t> _freeRanges;

    public:
        // Returns occupied offset or -1 if fails to occupy offset
        int32_t addElement(
            CustomData* pCustomData,
            CustomDataType type,
            size_t valueDataSize,
            const void* pValueData
        );

        template<typename T>
        void addNumericValue(CustomData* pCustomData, CustomDataType type, T value);
        void addStringValue(CustomData* pCustomData, const std::string& str);

        std::vector<CustomDataValue> getValues(int32_t offset) const;

        template<typename T>
        void updateNumericValue(
            CustomData* pCustomData,
            size_t valueIndex,
            T value
        );
        void updateStringValue(
            CustomData* pCustomData,
            size_t valueIndex,
            const std::string& str
        );

        template<typename T>
        T getNumericValue(const CustomData * const pCustomData, size_t valueIndex) const;
        std::string getStringValue(const CustomData * const pCustomData, size_t valueIndex) const;

        template<typename T>
        static T convert_numeric_value(const CustomDataValue& value);
        static std::string convert_string_value(const CustomDataValue& value);

        static size_t get_data_type_size(CustomDataType type);
    private:
        void erase(size_t offset, size_t totalDataSize);

        void updateElement(
            CustomData* pCustomData,
            size_t valueOffset,
            size_t valueDataSize,
            const void* pValueData
        );

        size_t valueOffsetToIndex(
            size_t customDataOffset,
            size_t elementCount,
            size_t valueOffset
        );

        bool isValueValid(CustomDataType dataType, size_t dataSize, const void* pData) const;
        void validateValue(CustomDataType dataType, size_t dataSize, const void* pData) const;

        size_t getAvailableOffset(size_t requiredSize) const;
        size_t getTotalSize(const CustomData * const pCustomData) const;
    };
}
