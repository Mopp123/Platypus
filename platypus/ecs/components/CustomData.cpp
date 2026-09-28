#include "CustomData.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::string custom_data_type_to_string(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::INT: return "INT";
            case CustomDataType::UINT: return "UINT";
            case CustomDataType::FLOAT: return "FLOAT";
            case CustomDataType::STRING: return "STRING";
            case CustomDataType::VECTOR2F: return "VECTOR2F";
            case CustomDataType::VECTOR3F: return "VECTOR3F";
            case CustomDataType::VECTOR4F: return "VECTOR4F";
        }
        return "Invalid type";
    }

    // _data layout:
    //  uint32_t elementCount
    //  CustomDataType[elementCount] types
    //  values[elementCount]
    //      NOTE: Values are in same order as types, so u can get
    //      the correct type for the value from there.
    int32_t CustomDataManager::addElement(
        CustomData* pCustomData,
        CustomDataType type,
        size_t valueDataSize,
        void* pValueData
    )
    {
        const size_t valueBaseSize = sizeof(CustomDataType) + sizeof(uint32_t);
        size_t requiredValueSize = valueBaseSize + get_data_type_size(type);
        if (type == CustomDataType::STRING)
            requiredValueSize = valueBaseSize + valueDataSize;

        const uint32_t oldElementCount = pCustomData->elementCount;
        const uint32_t newElementCount = oldElementCount + 1;
        std::vector<Value> values = getValues(pCustomData->offset);
        values.push_back(
            {
                type,
                static_cast<uint32_t>(requiredValueSize),
                pValueData
            }
        );

        const int32_t oldOffset = pCustomData->offset;
        const size_t oldTotalSize = getTotalSize(pCustomData);
        const size_t newTotalSize = oldTotalSize + requiredValueSize;
        if (oldOffset >= 0)
            erase(oldOffset, oldTotalSize);

        int32_t newOffset = -1;
        // TODO:
        // Figure out how to alloc new space if that's needed
        if (!_freeRanges.empty())
        {
            std::map<size_t, size_t>::const_iterator freeRangeIt;
            for (freeRangeIt = _freeRanges.begin(); freeRangeIt != _freeRanges.end(); ++freeRangeIt)
            {
                const size_t freeRangeOffset = freeRangeIt->first;
                const size_t freeRangeSize = freeRangeIt->second;
                // If free range offset was already at back
                //  ->just add the required additional space
                if (freeRangeOffset + freeRangeSize == _data.size())
                {
                    _data.resize(_data.size() + requiredValueSize);
                    newOffset = static_cast<int32_t>(freeRangeIt->first);
                    break;
                }
                else if (freeRangeSize >= newTotalSize)
                {
                    newOffset = static_cast<int32_t>(freeRangeIt->first);
                    break;
                }
            }
        }

        if (newOffset == -1)
        {
            // If offset was already at back
            //  ->just add the required additional space
            if (oldOffset + oldTotalSize == _data.size())
            {
                _data.resize(_data.size() + requiredValueSize);
                newOffset = oldOffset;
            }
            else
            {
                newOffset = _data.size();
                _data.resize(_data.size() + newTotalSize);
            }
        }
        pCustomData->elementCount = newElementCount;
        pCustomData->offset = newOffset;

        size_t valueOffset = 0;
        for (const Value& value : values)
        {
            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.type,
                sizeof(CustomDataType)
            );
            valueOffset += sizeof(CustomDataType);

            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.dataSize,
                sizeof(uint32_t)
            );
            valueOffset += sizeof(uint32_t);

            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.pData,
                value.dataSize
            );
            valueOffset += value.dataSize;
        }

        return newOffset;
    }

    void CustomDataManager::erase(size_t offset, size_t totalDataSize)
    {
        PLATYPUS_ASSERT(offset + totalDataSize <= _data.size());
        memset(_data.data() + offset, 0, totalDataSize);
        _freeRanges[offset] = totalDataSize;
    }

    void CustomDataManager::updateElement(
        const CustomData * const pCustomData,
        CustomDataType type,
        size_t valueOffset,
        size_t valueDataSize,
        void* pValueData
    )
    {
        const size_t customDataOffset = static_cast<const size_t>(pCustomData->offset);
        PLATYPUS_ASSERT(pCustomData->offset + customDataOffset <= _data.size());
        if (type == CustomDataType::STRING)
        {
            uint32_t currentStrSize = 0;
            memcpy(&currentStrSize, _data.data() + valueOffset, sizeof(uint32_t));
            if (currentStrSize >= valueDataSize)
            {
                memset(_data.data() + valueOffset, 0, currentStrSize);
            }
            else
            {
                Debug::log(
                    "Not enought size to update new string value. Resizing is required!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        else
        {
            if (!isValueValid(type, valueDataSize, pValueData))
            {
                Debug::log(
                    "Invalid value(type = " + custom_data_type_to_string(type) + " size = " + std::to_string(valueDataSize),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        uint8_t* pData = reinterpret_cast<uint8_t*>(_data.data());
        memcpy(pData + valueOffset, &type, sizeof(CustomDataType));
        valueOffset += sizeof(CustomDataType);

        const uint32_t valSize = static_cast<uint32_t>(valueDataSize);
        memcpy(
            pData + valueOffset,
            &valSize,
            sizeof(uint32_t)
        );
        valueOffset += sizeof(uint32_t);

        memcpy(
            pData + valueOffset,
            pValueData,
            valueDataSize
        );
    }

    bool CustomDataManager::isValueValid(CustomDataType dataType, size_t dataSize, void* pData)
    {
        if (dataType != CustomDataType::STRING)
            return dataSize == get_data_type_size(dataType);

        return true;
    }

    void CustomDataManager::validateValue(CustomDataType dataType, size_t dataSize, void* pData)
    {
        if (dataSize == 0)
        {
            Debug::log(
                "Value size was 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (dataType != CustomDataType::STRING)
        {
            if (!isValueValid(dataType, dataSize, pData))
            {
                Debug::log(
                    "Invalid value(type = " + custom_data_type_to_string(dataType) + " size = " + std::to_string(dataSize),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
    }

    size_t CustomDataManager::get_data_type_size(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::INT: return sizeof(int32_t);
            case CustomDataType::UINT: return sizeof(uint32_t);
            case CustomDataType::FLOAT: return sizeof(float);
            case CustomDataType::VECTOR2F: return sizeof(Vector2f);
            case CustomDataType::VECTOR3F: return sizeof(Vector3f);
            case CustomDataType::VECTOR4F: return sizeof(Vector4f);
        }
        return 0;
    }

    size_t CustomDataManager::getAvailableOffset(size_t requiredSize) const
    {
        if (_freeRanges.empty())
            return _data.size();

        std::map<size_t, size_t>::const_iterator it;
        for (it = _freeRanges.begin(); it != _freeRanges.end(); ++it)
        {
            const size_t freeRangeSize = it->second;
            if (freeRangeSize >= requiredSize)
                return it->first;
        }

        // We should never get here!
        Debug::log(
            "Failed to find available offset!",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);

        return 0;
    }

    size_t CustomDataManager::getTotalSize(const CustomData * const pCustomData) const
    {
        size_t size = 0;
        for (const Value& value : getValues(pCustomData))
            size += value.dataSize;

        return size;
    }

    std::vector<CustomDataManager::Value> CustomDataManager::getValues(int32_t offset) const
    {
        PLATYPUS_ASSERT(offset != -1);
        const size_t uOffset = static_cast<const size_t>(offset);
        PLATYPUS_ASSERT(uOffset + sizeof(uint32_t) <= _data.size());
        uint32_t elemCount = 0;
        const uint8_t* pData = reinterpret_cast<const uint8_t*>(_data.data());
        memcpy(&elemCount, pData + uOffset, sizeof(uint32_t));

        const size_t valuesBeginOffset = uOffset + sizeof(uint32_t);
        std::vector<Value> values(elemCount);
        size_t valueOffset = valuesBeginOffset;
        for (size_t i = 0; i < elemCount; ++i)
        {
            CustomDataType type;
            memcpy(&type, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            uint32_t valueSize = 0;
            memcpy(&valueSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            const void* pValueData = pData + valueOffset;
            values[i] = { type, valueSize, pValueData };

            valueOffset += valueSize;
        }
        return values;
    }
}
