#pragma once

#include "platypus/utils/Maths.hpp"
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

    struct CustomData
    {
        int32_t offset = -1;
        uint32_t elementCount;
    };

    CustomData* create_custom_data();
    std::string custom_data_type_to_string(CustomDataType type);


    class CustomDataManager
    {
    private:
        struct Value
        {
            CustomDataType type;
            uint32_t dataSize = 0;
            const void* pData = nullptr;
        };

        // _data layout:
        //  uint32_t elementCount
        //  values[elementCount]
        //      NOTE:
        //      *Values are in same order as types, so u can get
        //      the correct type for the value from there.
        std::vector<void*> _data;

        std::map<size_t, size_t> _freeRanges;

    public:

        static size_t get_data_type_size(CustomDataType type);
    private:
        // Returns occupied offset or -1 if fails to occupy offset
        int32_t addElement(
            CustomData* pCustomData,
            CustomDataType type,
            size_t valueDataSize,
            void* pValueData
        );

        void erase(size_t offset, size_t totalDataSize);

        void updateElement(
            const CustomData * const pCustomData,
            CustomDataType type,
            size_t valueOffset,
            size_t valueDataSize,
            void* pValueData
        );

        bool isValueValid(CustomDataType dataType, size_t dataSize, void* pData);
        void validateValue(CustomDataType dataType, size_t dataSize, void* pData);

        size_t getAvailableOffset(size_t requiredSize) const;
        size_t getTotalSize(const CustomData * const pCustomData) const;

        std::vector<Value> getValues(int32_t offset) const;
    };
}
