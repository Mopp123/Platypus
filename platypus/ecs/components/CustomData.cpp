#include "CustomData.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    CustomData* create_custom_data(
        entityID_t target,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_custom_data"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CUSTOM_DATA;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Transform component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        CustomData* pCustomData = reinterpret_cast<CustomData*>(pComponent);
        pCustomData->offset = -1;
        pCustomData->elementCount = 0;

        return pCustomData;
    }

    size_t get_serialized_custom_data_size(const CustomData * const pCustomData)
    {
        Debug::log("UNIMPLEMENTED!", PLATYPUS_CURRENT_FUNC_NAME, Debug::MessageType::PLATYPUS_ERROR);
        PLATYPUS_ASSERT(false);
        //CONTINUE HERE!
    }

    size_t get_serialized_custom_data_value_size(const CustomDataValue * const pCustomDataValue)
    {
        Debug::log("UNIMPLEMENTED!", PLATYPUS_CURRENT_FUNC_NAME, Debug::MessageType::PLATYPUS_ERROR);
        PLATYPUS_ASSERT(false);
    }

    /*
        Serialized format:
            ComponentType type
            uint32_t elementCount
            CustomDataValue(serialized form) values[elementCount]
    */
    std::vector<char> serialize(const CustomData * const pCustomData)
    {
        Debug::log("UNIMPLEMENTED!", PLATYPUS_CURRENT_FUNC_NAME, Debug::MessageType::PLATYPUS_ERROR);
        PLATYPUS_ASSERT(false);
    }

    std::vector<char> serialize(const CustomDataValue * const pCustomDataValue)
    {
        Debug::log("UNIMPLEMENTED!", PLATYPUS_CURRENT_FUNC_NAME, Debug::MessageType::PLATYPUS_ERROR);
        PLATYPUS_ASSERT(false);
    }

    void deserialize(
        Scene* pScene,
        CustomData** ppCustomData,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        Debug::log("UNIMPLEMENTED!", PLATYPUS_CURRENT_FUNC_NAME, Debug::MessageType::PLATYPUS_ERROR);
        PLATYPUS_ASSERT(false);
    }

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


    template<typename T>
    void CustomDataManager::addNumericValue(CustomData* pCustomData, CustomDataType type, T value)
    {
        if (type == CustomDataType::STRING)
        {
            Debug::log(
                "Attempted to add string as numeric value."
                "You'll need to use addString member func to add string.",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (addElement(
            pCustomData,
            type,
            get_data_type_size(type),
            &value
        ) == -1)
        {
            Debug::log(
                "Failed to add " + custom_data_type_to_string(type),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    template void CustomDataManager::addNumericValue<int32_t>(CustomData* pCustomData, CustomDataType type, int32_t value);
    template void CustomDataManager::addNumericValue<uint32_t>(CustomData* pCustomData, CustomDataType type, uint32_t value);
    template void CustomDataManager::addNumericValue<float>(CustomData* pCustomData, CustomDataType type, float value);
    template void CustomDataManager::addNumericValue<Vector2f>(CustomData* pCustomData, CustomDataType type, Vector2f value);
    template void CustomDataManager::addNumericValue<Vector3f>(CustomData* pCustomData, CustomDataType type, Vector3f value);
    template void CustomDataManager::addNumericValue<Vector4f>(CustomData* pCustomData, CustomDataType type, Vector4f value);

    void CustomDataManager::addStringValue(CustomData* pCustomData, const std::string& str)
    {
        if (addElement(
            pCustomData,
            CustomDataType::STRING,
            str.size(),
            reinterpret_cast<const void*>(str.data())
        ) == -1)
        {
            Debug::log(
                "Failed to add " + custom_data_type_to_string(CustomDataType::STRING),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
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
        const void* pValueData
    )
    {
        const size_t valueBaseSize = sizeof(CustomDataType) + sizeof(uint32_t) * 2;
        size_t storedValueSize = valueBaseSize + get_data_type_size(type);
        if (type == CustomDataType::STRING)
            storedValueSize = valueBaseSize + valueDataSize;

        const uint32_t oldElementCount = pCustomData->elementCount;
        const uint32_t newElementCount = oldElementCount + 1;
        const int32_t oldOffset = pCustomData->offset;
        const size_t oldTotalSize = getTotalSize(pCustomData);
        size_t newTotalSize = oldTotalSize + storedValueSize;

        // If old size was 0 -> need to also add space for the elemCount
        if (oldTotalSize == 0)
            newTotalSize += sizeof(uint32_t);

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
                    std::vector<uint8_t> oldData = _data;
                    _data.resize(_data.size() + storedValueSize);
                    memcpy(_data.data(), oldData.data(), oldData.size());
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
            std::vector<uint8_t> oldData = _data;
            // If offset was already at back
            //  ->just add the required additional space
            const size_t uOldOffset = static_cast<size_t>(oldOffset);
            if (oldOffset != -1 && uOldOffset + oldTotalSize == _data.size())
            {
                _data.resize(_data.size() + storedValueSize);
                memcpy(_data.data(), oldData.data(), oldData.size());
                newOffset = oldOffset;
            }
            else
            {
                newOffset = _data.size();
                _data.resize(_data.size() + newTotalSize);
                memcpy(_data.data(), oldData.data(), oldData.size());
            }
        }

        std::vector<CustomDataValue> values = getValues(oldOffset);
        values.push_back(
            {
                type,
                static_cast<uint32_t>(valueDataSize),
                static_cast<uint32_t>(valueDataSize),
                pValueData
            }
        );

        memcpy(
            _data.data() + newOffset,
            &newElementCount,
            sizeof(uint32_t)
        );
        size_t valueOffset = sizeof(uint32_t);
        for (const CustomDataValue& value : values)
        {
            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.type,
                sizeof(CustomDataType)
            );
            valueOffset += sizeof(CustomDataType);
            PLATYPUS_ASSERT(newOffset + valueOffset < _data.size());

            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.usedDataSize,
                sizeof(uint32_t)
            );
            valueOffset += sizeof(uint32_t);
            PLATYPUS_ASSERT(newOffset + valueOffset < _data.size());

            memcpy(
                _data.data() + newOffset + valueOffset,
                &value.maxDataSize,
                sizeof(uint32_t)
            );
            valueOffset += sizeof(uint32_t);
            PLATYPUS_ASSERT(newOffset + valueOffset < _data.size());

            memcpy(
                _data.data() + newOffset + valueOffset,
                value.pData,
                value.maxDataSize
            );
            valueOffset += value.maxDataSize;
            PLATYPUS_ASSERT(newOffset + valueOffset <= _data.size());
        }

        pCustomData->elementCount = newElementCount;
        pCustomData->offset = newOffset;

        return newOffset;
    }

    void CustomDataManager::erase(size_t offset, size_t totalDataSize)
    {
        PLATYPUS_ASSERT(offset + totalDataSize <= _data.size());
        //memset(_data.data() + offset, 0, totalDataSize);
        _freeRanges[offset] = totalDataSize;
    }

    void CustomDataManager::updateElement(
        const CustomData * const pCustomData,
        size_t valueOffset,
        size_t valueDataSize,
        const void* pValueData
    )
    {
        const size_t customDataOffset = static_cast<const size_t>(pCustomData->offset);
        PLATYPUS_ASSERT(pCustomData->offset + customDataOffset <= _data.size());

        uint8_t* pData = reinterpret_cast<uint8_t*>(_data.data());
        CustomDataType type;
        memcpy(&type, pData + valueOffset, sizeof(CustomDataType));
        valueOffset += sizeof(CustomDataType);
        const uint32_t valSize = static_cast<uint32_t>(valueDataSize);

        uint32_t maxValueSize = 0;
        memcpy(&maxValueSize, pData + valueOffset + sizeof(uint32_t), sizeof(uint32_t));

        if (type == CustomDataType::STRING)
        {
            uint32_t usedStrSize = 0;
            memcpy(&usedStrSize, pData + valueOffset, sizeof(uint32_t));
            if (maxValueSize >= valueDataSize)
            {
                memset(_data.data() + valueOffset + sizeof(uint32_t) * 2, 0, maxValueSize);
            }
            else
            {
                // TODO: Allow resizing!
                Debug::log(
                    "Not enought size to update new string value. Resizing is required! "
                    "TODO: Make resizing possible!",
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

        memcpy(
            pData + valueOffset,
            &valSize,
            sizeof(uint32_t)
        );
        valueOffset += sizeof(uint32_t);

        memcpy(
            pData + valueOffset,
            &maxValueSize,
            sizeof(uint32_t)
        );
        valueOffset += sizeof(uint32_t);

        memcpy(
            pData + valueOffset,
            pValueData,
            valueDataSize
        );
    }

    bool CustomDataManager::isValueValid(CustomDataType dataType, size_t dataSize, const void* pData) const
    {
        if (dataType != CustomDataType::STRING)
            return dataSize == get_data_type_size(dataType);

        return true;
    }

    void CustomDataManager::validateValue(CustomDataType dataType, size_t dataSize, const void* pData) const
    {
        if (dataSize == 0)
        {
            Debug::log(
                "CustomDataValue size was 0",
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

    std::vector<CustomDataValue> CustomDataManager::getValues(int32_t offset) const
    {
        if (offset == -1)
            return { };

        const size_t uOffset = static_cast<const size_t>(offset);
        PLATYPUS_ASSERT(uOffset + sizeof(uint32_t) <= _data.size());
        uint32_t elemCount = 0;
        const uint8_t* pData = reinterpret_cast<const uint8_t*>(_data.data());
        memcpy(&elemCount, pData + uOffset, sizeof(uint32_t));

        const size_t valuesBeginOffset = uOffset + sizeof(uint32_t);
        std::vector<CustomDataValue> values(elemCount);
        size_t valueOffset = valuesBeginOffset;
        for (size_t i = 0; i < elemCount; ++i)
        {
            CustomDataType type;
            memcpy(&type, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            uint32_t valueSize = 0;
            memcpy(&valueSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            uint32_t maxValueSize = 0;
            memcpy(&maxValueSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            const void* pValueData = pData + valueOffset;
            values[i] = { type, valueSize, maxValueSize, pValueData };

            valueOffset += maxValueSize;
        }
        return values;
    }

    template<typename T>
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        T value
    )
    {
        if (pCustomData->offset < 0)
        {
            Debug::log(
                "pCustomData offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (pCustomData->elementCount == 0)
        {
            Debug::log(
                "pCustomData elementCount was 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CustomDataType valueType;
        size_t valueOffset = pCustomData->offset + sizeof(uint32_t);
        uint32_t valueDataSize = 0;
        uint8_t* pData = _data.data();
        for (size_t i = 0; i < pCustomData->elementCount; ++i)
        {
            const size_t currentValueBaseOffset = valueOffset;

            memcpy(&valueType, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            memcpy(&valueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            uint32_t maxValueDataSize = 0;
            memcpy(&maxValueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            PLATYPUS_ASSERT(valueDataSize == maxValueDataSize);

            valueOffset += maxValueDataSize;

            if (i == valueIndex)
            {
                if (valueType == CustomDataType::STRING)
                {
                    Debug::log(
                        "Attempted to update string as numeric value!",
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
                valueOffset = currentValueBaseOffset;
                break;
            }

        }
        PLATYPUS_ASSERT(valueDataSize > 0);

        updateElement(
            pCustomData,
            valueOffset,
            valueDataSize,
            &value
        );
    }

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        int32_t value
    );

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        uint32_t value
    );

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        float value
    );

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        Vector2f value
    );

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        Vector3f value
    );

    template
    void CustomDataManager::updateNumericValue(
        CustomData* pCustomData,
        size_t valueIndex,
        Vector4f value
    );

    void CustomDataManager::updateStringValue(
        CustomData* pCustomData,
        size_t valueIndex,
        const std::string& str
    )
    {
        if (pCustomData->offset < 0)
        {
            Debug::log(
                "pCustomData offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (pCustomData->elementCount == 0)
        {
            Debug::log(
                "pCustomData elementCount was 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CustomDataType valueType;
        size_t valueOffset = pCustomData->offset + sizeof(uint32_t);
        uint32_t valueDataSize = 0;
        uint8_t* pData = _data.data();
        for (size_t i = 0; i < pCustomData->elementCount; ++i)
        {
            const size_t currentValueBaseOffset = valueOffset;

            memcpy(&valueType, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            memcpy(&valueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            uint32_t maxValueDataSize = 0;
            memcpy(&maxValueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            PLATYPUS_ASSERT(valueDataSize <= maxValueDataSize);

            valueOffset += maxValueDataSize;

            if (i == valueIndex)
            {
                if (valueType != CustomDataType::STRING)
                {
                    Debug::log(
                        "CustomDataValue type was: " + custom_data_type_to_string(valueType),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
                valueOffset = currentValueBaseOffset;
                break;
            }

        }
        PLATYPUS_ASSERT(valueDataSize > 0);

        updateElement(
            pCustomData,
            valueOffset,
            str.size(),
            str.data()
        );
    }

    template<typename T>
    T CustomDataManager::getNumericValue(const CustomData * const pCustomData, size_t valueIndex) const
    {
        if (pCustomData->offset < 0)
        {
            Debug::log(
                "pCustomData offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (pCustomData->elementCount == 0)
        {
            Debug::log(
                "pCustomData elementCount was 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        T result;
        size_t valueOffset = static_cast<size_t>(pCustomData->offset) + sizeof(uint32_t);
        const uint8_t* pData = _data.data();
        for (size_t i = 0; i < pCustomData->elementCount; ++i)
        {
            CustomDataType valueType;
            memcpy(&valueType, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            uint32_t valueDataSize;
            memcpy(&valueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            uint32_t maxValueDataSize;
            memcpy(&maxValueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            if (valueType != CustomDataType::STRING)
            {
                PLATYPUS_ASSERT(valueDataSize == maxValueDataSize);
            }
            else
            {
                PLATYPUS_ASSERT(valueDataSize <= maxValueDataSize);
            }

            if (i == valueIndex)
            {
                if (valueType == CustomDataType::STRING)
                {
                    Debug::log(
                        "Attempted to get string as numeric value from index: " + std::to_string(i),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return result;
                }
                if (!isValueValid(valueType, valueDataSize, pData + valueOffset))
                {
                    Debug::log(
                        "Invalid value for type: " + custom_data_type_to_string(valueType) + " "
                        "at index: " + std::to_string(i),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return result;
                }
                memcpy(&result, pData + valueOffset, get_data_type_size(valueType));
                return result;
            }
            valueOffset += maxValueDataSize;
        }

        Debug::log(
            "Failed to find value at index: " + std::to_string(valueIndex),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return result;
    }

    template int32_t CustomDataManager::getNumericValue<int32_t>(const CustomData * const pCustomData, size_t valueIndex) const;
    template uint32_t CustomDataManager::getNumericValue<uint32_t>(const CustomData * const pCustomData, size_t valueIndex) const;
    template float CustomDataManager::getNumericValue<float>(const CustomData * const pCustomData, size_t valueIndex) const;
    template Vector2f CustomDataManager::getNumericValue<Vector2f>(const CustomData * const pCustomData, size_t valueIndex) const;
    template Vector3f CustomDataManager::getNumericValue<Vector3f>(const CustomData * const pCustomData, size_t valueIndex) const;
    template Vector4f CustomDataManager::getNumericValue<Vector4f>(const CustomData * const pCustomData, size_t valueIndex) const;

    std::string CustomDataManager::getStringValue(const CustomData * const pCustomData, size_t valueIndex) const
    {
        if (pCustomData->offset < 0)
        {
            Debug::log(
                "pCustomData offset was -1",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (pCustomData->elementCount == 0)
        {
            Debug::log(
                "pCustomData elementCount was 0",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        size_t valueOffset = static_cast<size_t>(pCustomData->offset) + sizeof(uint32_t);
        const uint8_t* pData = _data.data();
        for (size_t i = 0; i < pCustomData->elementCount; ++i)
        {
            CustomDataType valueType;
            memcpy(&valueType, pData + valueOffset, sizeof(CustomDataType));
            valueOffset += sizeof(CustomDataType);

            uint32_t valueDataSize;
            memcpy(&valueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            uint32_t maxValueDataSize;
            memcpy(&maxValueDataSize, pData + valueOffset, sizeof(uint32_t));
            valueOffset += sizeof(uint32_t);

            PLATYPUS_ASSERT(valueDataSize <= maxValueDataSize);

            if (i == valueIndex)
            {
                if (valueType != CustomDataType::STRING)
                {
                    Debug::log(
                        "Attempted to get numeric value(type: " + custom_data_type_to_string(valueType) + ") "
                        "as string from index: " + std::to_string(i),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return "";
                }
                return {
                    reinterpret_cast<const char*>(pData + valueOffset),
                    valueDataSize
                };
            }

            valueOffset += maxValueDataSize;
        }

        Debug::log(
            "Failed to find value at index: " + std::to_string(valueIndex),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return "";
    }

    template<typename T>
    T CustomDataManager::convert_numeric_value(const CustomDataValue& value)
    {
        T retVal;
        memcpy(&retVal, value.pData, value.maxDataSize);
        return retVal;
    }

    template int32_t CustomDataManager::convert_numeric_value<int32_t>(const CustomDataValue& value);
    template uint32_t CustomDataManager::convert_numeric_value<uint32_t>(const CustomDataValue& value);
    template float CustomDataManager::convert_numeric_value<float>(const CustomDataValue& value);
    template Vector2f CustomDataManager::convert_numeric_value<Vector2f>(const CustomDataValue& value);
    template Vector3f CustomDataManager::convert_numeric_value<Vector3f>(const CustomDataValue& value);
    template Vector4f CustomDataManager::convert_numeric_value<Vector4f>(const CustomDataValue& value);

    std::string CustomDataManager::convert_string_value(const CustomDataValue& value)
    {
        if (value.type != CustomDataType::STRING)
        {
            Debug::log(
                "Invalid value type: " + custom_data_type_to_string(value.type),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return "Invalid value type";
        }

        return std::string(
            reinterpret_cast<const char*>(value.pData),
            static_cast<size_t>(value.usedDataSize)
        );
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
        if (pCustomData->elementCount == 0)
            return 0;

        size_t size = sizeof(uint32_t); // first the elem count
        const size_t baseValueSize = sizeof(CustomDataType) + sizeof(uint32_t) * 2;
        for (const CustomDataValue& value : getValues(pCustomData->offset))
            size += baseValueSize + value.maxDataSize;

        return size;
    }
}
