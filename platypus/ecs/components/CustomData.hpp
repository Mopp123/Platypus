#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
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


    struct CustomData
    {
        int32_t offset = -1;
        uint32_t elementCount = 0;
    };

    struct CustomDataValue
    {
        CustomDataType type;
        uint32_t usedDataSize = 0;
        uint32_t maxDataSize = 0;
        const void* pData = nullptr;
    };

    class Scene;
    CustomData* create_custom_data(
        entityID_t target,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    size_t get_serialized_custom_data_size(const CustomData * const pCustomData);
    size_t get_serialized_custom_data_value_size(const CustomDataValue * const pCustomDataValue);
    std::vector<char> serialize(const CustomData * const pCustomData);
    std::vector<char> serialize(const CustomDataValue * const pCustomDataValue);

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

        // _data layout:
        //  uint32_t elementCount
        //  values[elementCount]
        std::vector<uint8_t> _data;

        std::map<size_t, size_t> _freeRanges;

    public:
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
        // Returns occupied offset or -1 if fails to occupy offset
        int32_t addElement(
            CustomData* pCustomData,
            CustomDataType type,
            size_t valueDataSize,
            const void* pValueData
        );

        void erase(size_t offset, size_t totalDataSize);

        void updateElement(
            const CustomData * const pCustomData,
            size_t valueOffset,
            size_t valueDataSize,
            const void* pValueData
        );

        bool isValueValid(CustomDataType dataType, size_t dataSize, const void* pData) const;
        void validateValue(CustomDataType dataType, size_t dataSize, const void* pData) const;

        size_t getAvailableOffset(size_t requiredSize) const;
        size_t getTotalSize(const CustomData * const pCustomData) const;
    };
}
